#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "config.h"
#include "rfid.h"
#include "oled.h"

static const char *TAG = TAG_RFID;

// RC522 SPI handle
static spi_device_handle_t rc522_spi = NULL;
static uint32_t error_backoff_time = 5;  // Error backoff time in ms

// Authorized RFID UIDs
static uint8_t authorized_uids[MAX_AUTHORIZED_CARDS][UID_LENGTH] = {
    {0x43, 0xF4, 0x5A, 0x13}, // Card 1
    {0xF3, 0xF1, 0x11, 0x0E}, // Card 2
    {0x00, 0x00, 0x00, 0x00}, // Not used
    {0x00, 0x00, 0x00, 0x00}, // Not used
    {0x00, 0x00, 0x00, 0x00}  // Not used
};
static int authorized_uid_count = 2; // Default 2 cards programmed

// Forward declarations for internal functions
static esp_err_t rc522_write_reg(uint8_t reg, uint8_t data);
static uint8_t rc522_read_reg(uint8_t reg);
static esp_err_t rc522_set_bit_mask(uint8_t reg, uint8_t mask);
static esp_err_t rc522_clear_bit_mask(uint8_t reg, uint8_t mask);
static void rc522_reset_protocol_state(void);
static esp_err_t rc522_antenna_on(bool on);
static int rc522_card_write_read(uint8_t command, uint8_t *send_data, uint8_t send_len, 
                               uint8_t *back_data, uint8_t *back_len);
static bool rc522_wake_card(void);
static int rc522_request(uint8_t req_mode, uint8_t *tag_type);
static int rc522_anticoll(uint8_t *serial_num);
static int rc522_select_tag(uint8_t *serial_num);

/**
 * Debug register access operations
 */
static void debug_register_access(bool is_write, uint8_t reg, uint8_t value) {
    ESP_LOGD(TAG, "%s register: 0x%02X, value: 0x%02X", 
             is_write ? "Write" : "Read", reg, value);
}

/**
 * Write data to RC522 register
 */
static esp_err_t rc522_write_reg(uint8_t reg, uint8_t data)
{
    spi_transaction_t trans = {0};
    uint8_t tx_data[2] = {(reg << 1) & 0x7E, data}; // Address left shift 1 bit, LSB=0 for write
    
    trans.length = 16; // 16 bits (8 bits address + 8 bits data)
    trans.tx_buffer = tx_data;
    
    esp_err_t ret = spi_device_polling_transmit(rc522_spi, &trans);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Write register 0x%02X failed: %s", reg, esp_err_to_name(ret));
    } else {
        debug_register_access(true, reg, data);
    }
    return ret;
}

/**
 * Read data from RC522 register
 */
static uint8_t rc522_read_reg(uint8_t reg)
{
    spi_transaction_t trans = {0};
    uint8_t tx_data[2] = {((reg << 1) & 0x7E) | 0x80, 0x00}; // Address left shift 1 bit, MSB=1 for read
    uint8_t rx_data[2] = {0};
    
    trans.length = 16; // 16 bits
    trans.tx_buffer = tx_data;
    trans.rx_buffer = rx_data;
    
    esp_err_t ret = spi_device_polling_transmit(rc522_spi, &trans);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Read register 0x%02X failed: %s", reg, esp_err_to_name(ret));
        return 0;
    }
    
    debug_register_access(false, reg, rx_data[1]);
    return rx_data[1]; // Second byte is data
}

/**
 * Set register bits
 */
static esp_err_t rc522_set_bit_mask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp = rc522_read_reg(reg);
    return rc522_write_reg(reg, tmp | mask);
}

/**
 * Clear register bits
 */
static esp_err_t rc522_clear_bit_mask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp = rc522_read_reg(reg);
    return rc522_write_reg(reg, tmp & (~mask));
}

/**
 * Reset RC522 protocol state
 * Call before card operations to clear previous error states
 */
static void rc522_reset_protocol_state(void) {
    // Cancel current command
    rc522_write_reg(RC522_REG_COMMAND, RC522_CMD_IDLE);
    
    // Clear all interrupt request bits
    rc522_write_reg(RC522_REG_COM_IRQ, 0x7F);
    
    // Clear error flags
    rc522_write_reg(RC522_REG_ERROR, 0x00);
    
    // Clear FIFO buffer
    rc522_write_reg(RC522_REG_FIFO_LEVEL, 0x80);
    
    // Wait for changes to take effect
    vTaskDelay(pdMS_TO_TICKS(10));
}

/**
 * Control antenna power
 */
static esp_err_t rc522_antenna_on(bool on)
{
    if(on) {
        // Turn on antenna, TX1 and TX2 should output RF
        uint8_t value = rc522_read_reg(RC522_REG_TX_CONTROL);
        if((value & 0x03) != 0x03) {
            rc522_write_reg(RC522_REG_TX_CONTROL, value | 0x03);
        }
    } else {
        // Turn off antenna
        rc522_clear_bit_mask(RC522_REG_TX_CONTROL, 0x03);
    }
    
    // Verify antenna state
    vTaskDelay(pdMS_TO_TICKS(10)); // Wait for register to stabilize
    uint8_t value = rc522_read_reg(RC522_REG_TX_CONTROL);
    ESP_LOGD(TAG, "Antenna status: TX1=%d, TX2=%d (0x%02X)", 
             (value & 0x01) ? 1 : 0,
             (value & 0x02) ? 1 : 0,
             value);
    
    if(on && (value & 0x03) != 0x03) {
        ESP_LOGE(TAG, "Cannot turn on antenna, TX_CONTROL register value error!");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

/**
 * Verify and display key RC522 register configuration
 * (For debugging purposes)
 */
static void __attribute__((unused)) rc522_dump_registers(void) {
    ESP_LOGI(TAG, "RC522 key register status:");
    ESP_LOGI(TAG, "CMD:\t\t0x%02X", rc522_read_reg(RC522_REG_COMMAND));
    ESP_LOGI(TAG, "MODE:\t\t0x%02X", rc522_read_reg(RC522_REG_MODE));
    ESP_LOGI(TAG, "TX_CONTROL:\t0x%02X", rc522_read_reg(RC522_REG_TX_CONTROL));
    ESP_LOGI(TAG, "TX_ASK:\t\t0x%02X", rc522_read_reg(RC522_REG_TX_ASK));
    ESP_LOGI(TAG, "TX_MODE:\t0x%02X", rc522_read_reg(RC522_REG_TX_MODE));
    ESP_LOGI(TAG, "RX_MODE:\t0x%02X", rc522_read_reg(RC522_REG_RX_MODE));
    ESP_LOGI(TAG, "RF_CFG:\t\t0x%02X", rc522_read_reg(RC522_REG_RF_CFG));
    ESP_LOGI(TAG, "VERSION:\t0x%02X", rc522_read_reg(RC522_REG_VERSION));
}

/**
 * RC522 communication with card
 */
static int rc522_card_write_read(uint8_t command, uint8_t *send_data, uint8_t send_len, uint8_t *back_data, uint8_t *back_len)
{
    int status = RC522_OK;
    uint8_t irq_wait = 0x00;
    uint8_t last_bits = 0;
    uint8_t n = 0;
    uint32_t i = 0;
    
    // Set interrupt wait conditions based on command
    if(command == RC522_CMD_TRANSCEIVE) {
        irq_wait = 0x30;     // RxIRq and IdleIRq
    } else if(command == RC522_CMD_MF_AUTHENT) {
        irq_wait = 0x10;     // IdleIRq
    }
    
    // Configure additional register settings to improve stability
    rc522_write_reg(RC522_REG_CONTROL, 0x80);  // Stop timer if non-zero value in FIFO
    
    // Clear FIFO buffer
    rc522_write_reg(RC522_REG_FIFO_LEVEL, 0x80);
    
    // Cancel current command
    rc522_write_reg(RC522_REG_COMMAND, RC522_CMD_IDLE);
    
    // Clear all interrupt request bits
    rc522_write_reg(RC522_REG_COM_IRQ, 0x7F);
    
    // Write data to FIFO
    for(i = 0; i < send_len; i++) {
        rc522_write_reg(RC522_REG_FIFO_DATA, send_data[i]);
    }
    
    // Execute command
    rc522_write_reg(RC522_REG_COMMAND, command);
    
    // If transmit command, start transmission
    if(command == RC522_CMD_TRANSCEIVE) {
        rc522_set_bit_mask(RC522_REG_BIT_FRAMING, 0x80);
    }
    
    // Wait for command completion with increased timeout
    uint16_t timeout_count = 3000; // Increased timeout
    do {
        vTaskDelay(pdMS_TO_TICKS(1)); // Short delay to avoid CPU overuse
        n = rc522_read_reg(RC522_REG_COM_IRQ);
        timeout_count--;
    } while((--timeout_count > 0) && !(n & 0x01) && !(n & irq_wait));
    
    // Stop transmission
    rc522_clear_bit_mask(RC522_REG_BIT_FRAMING, 0x80);
    
    // Check timeout
    if(timeout_count == 0) {
        ESP_LOGE(TAG, "Command timeout: 0x%02X", command);
        return RC522_ERR_TIMEOUT;
    }
    
    // Check for errors
    uint8_t error = rc522_read_reg(RC522_REG_ERROR);
    
    if(error & 0x13) { // BufferOvfl, ColErr or ParityErr
        ESP_LOGE(TAG, "Communication error: 0x%02X", error);
        return RC522_ERR_PROTOCOL;
    }
    
    status = RC522_OK;
    
    // Check data reception
    if(n & irq_wait) { // Command completed
        if(command == RC522_CMD_TRANSCEIVE) {
            n = rc522_read_reg(RC522_REG_FIFO_LEVEL);
            last_bits = rc522_read_reg(RC522_REG_CONTROL) & 0x07;
            
            if(last_bits) {
                *back_len = (n - 1) * 8 + last_bits;
            } else {
                *back_len = n * 8;
            }
            
            if(n == 0) {
                n = 1;
            }
            if(n > 16) {
                n = 16;
            }
            
            // Read received data
            for(i = 0; i < n; i++) {
                back_data[i] = rc522_read_reg(RC522_REG_FIFO_DATA);
            }
        }
    } else {
        // Don't log every single warning to prevent log flooding
        // Only log warn messages for specific errors
        if ((n & 0x01) == 0) { // Timer interrupt - actual problem
            ESP_LOGW(TAG, "Command timed out, IRQ: 0x%02X", n);
        } else {
            ESP_LOGD(TAG, "Command did not complete normally, IRQ: 0x%02X", n);
        }
        status = RC522_ERR;
    }
    
    return status;
}

/**
 * Card wake-up sequence
 * Send multiple wake commands to help card stabilize
 */
static bool rc522_wake_card(void) {
    uint8_t back_len = 0;
    uint8_t buffer[16] = {0};
    
    // Ensure antenna is at full power
    rc522_antenna_on(true);
    
    // Give card enough time to collect energy from RF field
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Reset protocol state, clear previous errors
    rc522_reset_protocol_state();
    
    // Send wake-up command three times to help card initialize
    for(int i = 0; i < 3; i++) {
        // Set appropriate bit frame adjustment
        rc522_write_reg(RC522_REG_BIT_FRAMING, 0x07);  // Send only 7 bits
        
        // Send wake command
        buffer[0] = PICC_CMD_WUPA;
        
        // Execute command (ignoring status, just trying to wake up card)
        uint8_t tmpLen = back_len;
        rc522_card_write_read(RC522_CMD_TRANSCEIVE, buffer, 1, buffer, &tmpLen);
        
        // Wait briefly for card to process
        vTaskDelay(pdMS_TO_TICKS(20));
        
        // Clear any error states
        rc522_write_reg(RC522_REG_ERROR, 0x00);
        rc522_write_reg(RC522_REG_COM_IRQ, 0x7F);
    }
    
    return true;
}

/**
 * Request card (optimized version)
 * req_mode: Request mode PICC_CMD_REQA or PICC_CMD_WUPA
 * tag_type: Returns card type
 * Returns operation status
 */
static int rc522_request(uint8_t req_mode, uint8_t *tag_type)
{
    int status;
    uint8_t back_len = 0;
    uint8_t buffer[16] = {0};
    
    // Reset protocol state
    rc522_reset_protocol_state();
    
    // Configure key registers with more stable settings
    rc522_write_reg(RC522_REG_TX_MODE, 0x00);  // Disable CRC
    rc522_write_reg(RC522_REG_RX_MODE, 0x00);  // Disable CRC
    rc522_write_reg(RC522_REG_MOD_WIDTH, 0x26); // Set modulation width for better stability
    rc522_write_reg(RC522_REG_RF_CFG, 0x70);   // Increase RF power to highest setting
    
    // Set work mode - send only 7 bits
    rc522_write_reg(RC522_REG_BIT_FRAMING, 0x07);
    
    // Write request command
    buffer[0] = req_mode;
    
    // Execute transceive command - try up to 3 times
    for (int retry = 0; retry < 3; retry++) {
        // Reset comm flags before each attempt
        rc522_write_reg(RC522_REG_COM_IRQ, 0x7F);
        rc522_write_reg(RC522_REG_ERROR, 0x00);
        
        status = rc522_card_write_read(RC522_CMD_TRANSCEIVE, buffer, 1, buffer, &back_len);
    
        if(status == RC522_OK && back_len == 16) { // Should receive 16 bits of data
            ESP_LOGI(TAG, "Received raw data: [0]=%02X [1]=%02X", buffer[0], buffer[1]);
            *tag_type = ((buffer[0] << 8) | buffer[1]) & 0xFFFF;
            ESP_LOGI(TAG, "Detected card type: 0x%04X", *tag_type);
            return RC522_OK;  // Success, exit retry loop
        }
        
        if(status != RC522_OK) {
            // If error, wait and try again
            ESP_LOGD(TAG, "Retry %d: Card detection failed, status=%d", retry + 1, status);
            vTaskDelay(pdMS_TO_TICKS(10));
        } else if(back_len != 16) {
            ESP_LOGD(TAG, "Retry %d: Invalid back_len=%d (expected 16 bits)", retry + 1, back_len);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    
    // If we got here, all retries failed
    ESP_LOGD(TAG, "Card detection failed after retries");
    return RC522_ERR;
}

/**
 * Anti-collision detection, get card serial number (optimized version)
 * serial_num: Returns card serial number, 4 bytes
 * Returns operation status
 */
static int rc522_anticoll(uint8_t *serial_num)
{
    int status;
    uint8_t i, serial_num_check = 0;
    uint8_t back_len = 0;
    uint8_t buffer[16] = {0};
    
    // Reset protocol state
    rc522_reset_protocol_state();
    
    // Reset bit count
    rc522_write_reg(RC522_REG_BIT_FRAMING, 0x00);
    
    // Disable CRC check
    rc522_write_reg(RC522_REG_TX_MODE, 0x00);
    rc522_write_reg(RC522_REG_RX_MODE, 0x00);
    
    // Disable encryption
    rc522_clear_bit_mask(RC522_REG_STATUS2, 0x08);
    
    // Clear collision error flag
    rc522_clear_bit_mask(RC522_REG_COLL, 0x80);
    
    // Anti-collision command
    buffer[0] = PICC_CMD_ANTICOLL_1;
    buffer[1] = 0x20; // NVB=0x20, indicates UID unknown, begin full anti-collision process
    
    status = rc522_card_write_read(RC522_CMD_TRANSCEIVE, buffer, 2, buffer, &back_len);
    
    if(status == RC522_OK) {
        // Check if enough data received
        if(back_len < 32) {  // Need at least 4 bytes UID
            ESP_LOGE(TAG, "Not enough data received, only %d bits", back_len);
            return RC522_ERR;
        }
        
        // Verify data
        for(i = 0; i < 4; i++) {
            serial_num[i] = buffer[i];
            serial_num_check ^= buffer[i]; // Calculate checksum
        }
        
        // Check checksum
        if(serial_num_check != buffer[4]) {
            ESP_LOGE(TAG, "UID checksum error: calculated=%02X, received=%02X", serial_num_check, buffer[4]);
            return RC522_ERR_WRONG_UID_CK;
        }
    }
    
    // Re-enable encryption
    rc522_set_bit_mask(RC522_REG_STATUS2, 0x08);
    
    return status;
}

/**
 * Select card, verify UID (optimized version)
 * serial_num: Card serial number
 * Returns operation status
 */
static int rc522_select_tag(uint8_t *serial_num)
{
    int status;
    uint8_t i;
    uint8_t back_len = 0;
    uint8_t buffer[16] = {0};
    
    // Reset protocol state
    rc522_reset_protocol_state();
    
    // Activate CRC check
    rc522_write_reg(RC522_REG_TX_MODE, 0x80);  // Enable TX CRC
    rc522_write_reg(RC522_REG_RX_MODE, 0x80);  // Enable RX CRC
    
    // Select command
    buffer[0] = PICC_CMD_SELECT_1;
    buffer[1] = 0x70; // NVB, complete bit count (7 bytes)
    
    // Copy UID
    for(i = 0; i < 4; i++) {
        buffer[i+2] = serial_num[i];
    }
    
    // Calculate BCC
    buffer[6] = serial_num[0] ^ serial_num[1] ^ serial_num[2] ^ serial_num[3];
    
    // Execute select command
    status = rc522_card_write_read(RC522_CMD_TRANSCEIVE, buffer, 7, buffer, &back_len);
    
    if(status == RC522_OK && back_len == 8) { // SAK should be 1 byte, plus CRC is 24 bits
        ESP_LOGI(TAG, "Card selection successful, SAK value: 0x%02X", buffer[0]);
        return RC522_OK;
    } else {
        ESP_LOGW(TAG, "Card selection failed: status=%d, return length=%d", status, back_len);
        return RC522_ERR;
    }
}

/**
 * Initialize the RC522 RFID module
 */
esp_err_t rfid_init(void)
{
    ESP_LOGI(TAG, "RC522 initialization starting...");
    ESP_LOGI(TAG, "Using pins: MISO=%d, MOSI=%d, SCLK=%d, CS=%d, RST=%d", 
             RC522_SPI_MISO, RC522_SPI_MOSI, RC522_SPI_SCLK, RC522_SPI_CS, RC522_RST);
    
    // Configure SPI bus
    spi_bus_config_t buscfg = {
        .miso_io_num = RC522_SPI_MISO,
        .mosi_io_num = RC522_SPI_MOSI,
        .sclk_io_num = RC522_SPI_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32, // Max transfer 32 bytes at a time
    };
    
    ESP_LOGI(TAG, "Initializing SPI bus...");
    esp_err_t ret = spi_bus_initialize(RC522_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus initialization failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure SPI device - lower clock speed for stability
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000,    // Lowered to 1MHz for stability
        .mode = 0,                   // SPI mode 0
        .spics_io_num = RC522_SPI_CS,
        .queue_size = 7,
        .command_bits = 0,
        .address_bits = 0,           // No address bits, we send in data
        .dummy_bits = 0,
    };
    
    ESP_LOGI(TAG, "Adding RC522 device to SPI bus...");
    ret = spi_bus_add_device(RC522_SPI_HOST, &devcfg, &rc522_spi);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Adding RC522 device failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure reset pin (RC522_RST)
    gpio_config_t rst_conf = {
        .pin_bit_mask = (1ULL << RC522_RST),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    ESP_LOGI(TAG, "Configuring RC522 reset pin...");
    ret = gpio_config(&rst_conf);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Configuring reset pin failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Perform hardware reset - extended reset time
    ESP_LOGI(TAG, "Executing hardware reset...");
    gpio_set_level(RC522_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(50));  // Extended low time
    gpio_set_level(RC522_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100)); // Extended wait time
    
    // Software reset RC522
    ESP_LOGI(TAG, "Executing software reset...");
    rc522_write_reg(RC522_REG_COMMAND, RC522_CMD_SOFT_RESET);
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for software reset to complete
    
    // Check connection
    ESP_LOGI(TAG, "Verifying RC522 communication...");
    uint8_t version = rc522_read_reg(RC522_REG_VERSION);
    ESP_LOGI(TAG, "RC522 version register value: 0x%02X", version);
    
    if(version == 0x00 || version == 0xFF) {
        ESP_LOGE(TAG, "RC522 communication error, detected value invalid (0x%02X)!", version);
        ESP_LOGE(TAG, "Please check: 1.Pin connections 2.Power stability 3.SPI configuration");
        return ESP_FAIL;
    }
    
    // Configure RC522 work mode
    ESP_LOGI(TAG, "Configuring RC522 work mode...");
    
    // Timer configuration
    rc522_write_reg(RC522_REG_T_MODE, 0x8D);      // TAuto=1; Timer works, counter auto-increments every TPreScaler clock cycles
    rc522_write_reg(RC522_REG_T_PRESCALER, 0x3E); // Timer prescaler setting
    rc522_write_reg(RC522_REG_T_RELOAD_L, 30);    // Timer reload low byte
    rc522_write_reg(RC522_REG_T_RELOAD_H, 0);     // Timer reload high byte
    
    // Transmit and receive settings
    rc522_write_reg(RC522_REG_TX_ASK, 0x40);      // Set 100% ASK modulation
    rc522_write_reg(RC522_REG_MODE, 0x3D);        // CRC init value 0x6363, use ISO14443A frame format
    
    // RF configuration - adjust transmit power
    rc522_write_reg(RC522_REG_RF_CFG, 0x68);      // Medium field strength, suitable for most cards
    
    // Receive settings
    rc522_write_reg(RC522_REG_RX_THRESHOLD, 0x84); // Lower receive threshold for better sensitivity
    
    // Start antenna
    ESP_LOGI(TAG, "Starting antenna...");
    rc522_antenna_on(true);
    
    ESP_LOGI(TAG, "RC522 initialization complete");
    return ESP_OK;
}

/**
 * Detect card and read UID
 */
bool rfid_read_card_uid(uint8_t *uid, uint8_t *uid_size)
{
    int status;
    uint8_t card_type[2];
    static uint16_t error_count = 0;
    
    // Prevent too many error outputs
    bool silent_mode = (error_count > 50);
    
    // Execute card wake sequence to help card stabilize
    rc522_wake_card();
    
    // 1. Request card - try REQA
    status = rc522_request(PICC_CMD_REQA, card_type);
    if(status != RC522_OK) {
        // If REQA fails, try WUPA
        status = rc522_request(PICC_CMD_WUPA, card_type);
        if(status != RC522_OK) {
            if(!silent_mode) {
                ESP_LOGD(TAG, "Card detection failed: %d", status);
                error_count++;
                if(error_count == 50) {
                    ESP_LOGW(TAG, "Entering silent mode, no more card detection errors will be printed");
                }
            }
            // Increase backoff time after error
            error_backoff_time = (error_backoff_time * 3) / 2;  // Increase by 50%
            if(error_backoff_time > 100) {
                error_backoff_time = 100;  // Max 100ms
            }
            vTaskDelay(pdMS_TO_TICKS(error_backoff_time));
            return false;
        }
    }
    
    // Success - reset error count and backoff time
    error_count = 0; 
    error_backoff_time = 5;
    
    // Reset protocol state, prepare for next operation
    rc522_reset_protocol_state();
    
    // 2. Anti-collision
    status = rc522_anticoll(uid);
    if(status != RC522_OK) {
        ESP_LOGW(TAG, "Anti-collision failed: %d", status);
        return false;
    }
    
    // Reset protocol state, prepare for next operation
    rc522_reset_protocol_state();
    
    // 3. Select card
    status = rc522_select_tag(uid);
    if(status != RC522_OK) {
        ESP_LOGW(TAG, "Card selection failed: %d", status);
        return false;
    }
    
    *uid_size = 4; // Mifare card UID size is 4 bytes
    
    return true;
}

/**
 * Check if UID is in authorized list
 */
bool rfid_check_authorized_uid(uint8_t *uid, uint8_t uid_size)
{
    // Check if UID matches any authorized UIDs
    for (int i = 0; i < authorized_uid_count && i < MAX_AUTHORIZED_CARDS; i++) {
        bool match = true;
        for (int j = 0; j < uid_size && j < UID_LENGTH; j++) {
            if (authorized_uids[i][j] != uid[j]) {
                match = false;
                break;
            }
        }
        
        if (match) {
            ESP_LOGI(TAG, "Authorized card verification successful");
            return true;
        }
    }
    
    ESP_LOGI(TAG, "Unauthorized card");
    return false;
}

/**
 * Add UID to authorized list
 */
bool rfid_add_authorized_uid(uint8_t *uid, uint8_t uid_size)
{
    if (authorized_uid_count < MAX_AUTHORIZED_CARDS) {
        // Copy UID to authorized list
        for (int i = 0; i < uid_size && i < UID_LENGTH; i++) {
            authorized_uids[authorized_uid_count][i] = uid[i];
        }
        authorized_uid_count++;
        ESP_LOGI(TAG, "Added authorized card, current count: %d", authorized_uid_count);
        return true;
    } else {
        ESP_LOGW(TAG, "Maximum authorized cards reached (%d)", MAX_AUTHORIZED_CARDS);
        return false;
    }
}

/**
 * Initialize the list of authorized UIDs
 */
void rfid_init_authorized_list(void)
{
    // Default list already initialized in static array
    ESP_LOGI(TAG, "Authorized card list initialized with %d cards", authorized_uid_count);
}

/**
 * Get current authorized UID count
 */
int rfid_get_authorized_count(void)
{
    return authorized_uid_count;
}

/**
 * RFID task for continuously scanning for cards
 */
void rfid_task(void *arg)
{
    ESP_LOGI(TAG, "RFID task started");
    
    uint16_t scan_count = 0;
    uint32_t retry_interval = 100; // Scan interval (ms)
    
    // Function pointers for door lock/unlock callbacks
    void (**callbacks)(void) = (void (**)(void))arg;
    void (*unlock_door_callback)(void) = callbacks[0];
    void (*lock_door_callback)(void) = callbacks[1];
    
    while(1) {
        uint8_t uid[UID_LENGTH] = {0}; // Buffer for card UID
        uint8_t uid_size = 0;
        
        scan_count++;
        if(scan_count % 500 == 0) {
            ESP_LOGD(TAG, "Continuous scanning... (%d)", scan_count / 500);
            
            // Periodically perform wake sequence to ensure system stability
            if(scan_count % 2000 == 0) {
                rc522_wake_card();
            }
        }
        
        if(rfid_read_card_uid(uid, &uid_size)) {
            char uid_str[32] = {0};
            for(int i = 0; i < uid_size; i++) {
                sprintf(uid_str + strlen(uid_str), "%02X ", uid[i]);
            }
            ESP_LOGI(TAG, "Card detected: %s", uid_str);
            
            // Display UID on OLED
            oled_show_uid(uid, uid_size);
            
            // Check if authorized card
            if (rfid_check_authorized_uid(uid, uid_size)) {
                oled_show_message("Card authorized!", 1000);
                
                // If unlock callback is provided, call it
                if (unlock_door_callback != NULL) {
                    unlock_door_callback();
                    
                    // Auto-lock after 5 seconds
                    oled_show_message("Auto-lock in 5s", 0);
                    vTaskDelay(pdMS_TO_TICKS(5000));
                    
                    if (lock_door_callback != NULL) {
                        lock_door_callback();
                    }
                }
            } else {
                oled_show_message("Card denied!", 2000);
            }
            
            // Prevent repeated reading of the same card, delay 2 seconds
            vTaskDelay(pdMS_TO_TICKS(2000));
            
            // Reset counters
            scan_count = 0;
            retry_interval = 100;
            
            // Update OLED display - clear UID display
            oled_clear_page(5);
            oled_refresh();
        }
        
        // Short delay to avoid CPU overuse
        vTaskDelay(pdMS_TO_TICKS(retry_interval));
        
        // Dynamically adjust scan interval, reduce frequency if no card detected for a long time
        if(scan_count > 1000 && retry_interval < 500) {
            retry_interval = 500;
            ESP_LOGD(TAG, "No card detected for a long time, reducing scan frequency");
        }
    }
}