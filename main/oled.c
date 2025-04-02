#include <stdio.h>
#include <string.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "config.h"
#include "oled.h"

static const char *TAG = TAG_OLED;

// OLED buffer
static uint8_t oled_buffer[OLED_WIDTH * OLED_PAGES] = {0};

// Mutex to protect OLED operations
static SemaphoreHandle_t oled_mutex = NULL;

// Basic 5x8 font for OLED display
static const uint8_t font5x8[96][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x3E, 0x14, 0x3E, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x56, 0x20, 0x50}, // &
    {0x00, 0x08, 0x07, 0x03, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x2A, 0x1C, 0x7F, 0x1C, 0x2A}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x80, 0x70, 0x30, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x00, 0x60, 0x60, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x72, 0x49, 0x49, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x49, 0x4D, 0x33}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x31}, // 6
    {0x41, 0x21, 0x11, 0x09, 0x07}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x46, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x00, 0x14, 0x00, 0x00}, // :
    {0x00, 0x40, 0x34, 0x00, 0x00}, // ;
    {0x00, 0x08, 0x14, 0x22, 0x41}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x59, 0x09, 0x06}, // ?
    {0x3E, 0x41, 0x5D, 0x59, 0x4E}, // @
    {0x7C, 0x12, 0x11, 0x12, 0x7C}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x41, 0x3E}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x41, 0x51, 0x73}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x1C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x26, 0x49, 0x49, 0x49, 0x32}, // S
    {0x03, 0x01, 0x7F, 0x01, 0x03}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x03, 0x04, 0x78, 0x04, 0x03}, // Y
    {0x61, 0x59, 0x49, 0x4D, 0x43}, // Z
    {0x00, 0x7F, 0x41, 0x41, 0x41}, // [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // '\'
    {0x00, 0x41, 0x41, 0x41, 0x7F}, // ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // _
    {0x00, 0x03, 0x07, 0x08, 0x00}, // `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // a
    {0x7F, 0x28, 0x44, 0x44, 0x38}, // b
    {0x38, 0x44, 0x44, 0x44, 0x28}, // c
    {0x38, 0x44, 0x44, 0x28, 0x7F}, // d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // e
    {0x00, 0x08, 0x7E, 0x09, 0x02}, // f
    {0x18, 0xA4, 0xA4, 0xA4, 0x7C}, // g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
    {0x20, 0x40, 0x40, 0x3D, 0x00}, // j
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
    {0x7C, 0x04, 0x78, 0x04, 0x78}, // m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // o
    {0xFC, 0x18, 0x24, 0x24, 0x18}, // p
    {0x18, 0x24, 0x24, 0x18, 0xFC}, // q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // r
    {0x48, 0x54, 0x54, 0x54, 0x24}, // s
    {0x04, 0x04, 0x3F, 0x44, 0x24}, // t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // x
    {0x4C, 0x90, 0x90, 0x90, 0x7C}, // y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // z
    {0x00, 0x08, 0x36, 0x41, 0x00}, // {
    {0x00, 0x00, 0x77, 0x00, 0x00}, // |
    {0x00, 0x41, 0x36, 0x08, 0x00}, // }
    {0x02, 0x01, 0x02, 0x04, 0x02}, // ~
    {0x00, 0x00, 0x00, 0x00, 0x00}  // DEL
};

/**
 * Initialize I2C for OLED display
 */
static esp_err_t i2c_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        return err;
    }
    
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 
                              I2C_MASTER_RX_BUF_DISABLE, 
                              I2C_MASTER_TX_BUF_DISABLE, 0);
}

/**
 * Write a command to the OLED with retry
 */
static esp_err_t oled_write_cmd(uint8_t cmd)
{
    uint8_t command[2] = {OLED_CONTROL_BYTE_CMD_SINGLE, cmd};
    esp_err_t ret = ESP_FAIL;
    
    for (int retry = 0; retry < OLED_MAX_RETRY; retry++) {
        ret = i2c_master_write_to_device(
            I2C_MASTER_NUM, 
            OLED_ADDR, 
            command, 
            sizeof(command), 
            pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS)
        );
        
        if (ret == ESP_OK) {
            return ESP_OK;
        }
        
        ESP_LOGW(TAG, "Retry %d: Command 0x%02X failed: %s", 
                 retry + 1, cmd, esp_err_to_name(ret));
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    return ret;
}

/**
 * Write data to the OLED with retry
 */
static esp_err_t oled_write_data(uint8_t* data, size_t size)
{
    if (size == 0) {
        return ESP_OK;
    }
    
    // Add control byte for data
    uint8_t *buffer = malloc(size + 1);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for OLED data");
        return ESP_ERR_NO_MEM;
    }
    
    buffer[0] = OLED_CONTROL_BYTE_DATA_STREAM;
    memcpy(buffer + 1, data, size);
    
    esp_err_t ret = ESP_FAIL;
    for (int retry = 0; retry < OLED_MAX_RETRY; retry++) {
        ret = i2c_master_write_to_device(
            I2C_MASTER_NUM, 
            OLED_ADDR, 
            buffer, 
            size + 1, 
            pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS)
        );
        
        if (ret == ESP_OK) {
            break;
        }
        
        ESP_LOGW(TAG, "Retry %d: Data write of %d bytes failed: %s", 
                 retry + 1, size, esp_err_to_name(ret));
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    free(buffer);
    return ret;
}

/**
 * Initialize OLED display
 */
esp_err_t oled_init(void)
{
    ESP_LOGI(TAG, "Initializing OLED display...");
    
    // Create mutex for OLED operations
    oled_mutex = xSemaphoreCreateMutex();
    if (oled_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create OLED mutex");
        return ESP_FAIL;
    }
    
    // Initialize I2C
    esp_err_t ret = i2c_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initialize OLED
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for OLED power stabilization
    
    // SSD1306 initialization sequence
    ret = oled_write_cmd(OLED_CMD_DISPLAY_OFF);            // 0xAE
    if (ret != ESP_OK) goto init_error;
    
    ret = oled_write_cmd(OLED_CMD_SET_DISPLAY_CLK_DIV);    // 0xD5
    if (ret != ESP_OK) goto init_error;
    ret = oled_write_cmd(0x80);                            // Recommended clock divider
    if (ret != ESP_OK) goto init_error;
    
    ret = oled_write_cmd(OLED_CMD_SET_MUX_RATIO);          // 0xA8
    if (ret != ESP_OK) goto init_error;
    ret = oled_write_cmd(0x3F);                            // 64 line display (0x3F)
    if (ret != ESP_OK) goto init_error;
    
    ret = oled_write_cmd(OLED_CMD_SET_DISPLAY_OFFSET);     // 0xD3
    if (ret != ESP_OK) goto init_error;
    ret = oled_write_cmd(0x00);                            // No offset
    if (ret != ESP_OK) goto init_error;
    
    ret = oled_write_cmd(OLED_CMD_SET_START_LINE | 0x00);  // Set start line 0x40
    if (ret != ESP_OK) goto init_error;
    
    ret = oled_write_cmd(OLED_CMD_CHARGE_PUMP);            // 0x8D
    if (ret != ESP_OK) goto init_error;
    ret = oled_write_cmd(0x14);                            // Enable internal charge pump
    if (ret != ESP_OK) goto init_error;
    
    ret = oled_write_cmd(OLED_CMD_SET_MEMORY_ADDR_MODE);   // 0x20
    if (ret != ESP_OK) goto init_error;
    ret = oled_write_cmd(0x00);                            // Horizontal addressing mode
    if (ret != ESP_OK) goto init_error;
    
    ret = oled_write_cmd(OLED_CMD_SET_SEGMENT_REMAP | 0x01); // 0xA1 Column address map inverted
    if (ret != ESP_OK) goto init_error;
    
    ret = oled_write_cmd(OLED_CMD_SET_COM_SCAN_MODE | 0x08); // 0xC8 Row scan direction inverted
    if (ret != ESP_OK) goto init_error;
    
    ret = oled_write_cmd(OLED_CMD_SET_COM_PIN_MAP);        // 0xDA
    if (ret != ESP_OK) goto init_error;
    ret = oled_write_cmd(0x12);                            // Recommended pin config for 128x64
    if (ret != ESP_OK) goto init_error;
    
    ret = oled_write_cmd(OLED_CMD_SET_CONTRAST);           // 0x81
    if (ret != ESP_OK) goto init_error;
    ret = oled_write_cmd(0xCF);                            // Set higher contrast
    if (ret != ESP_OK) goto init_error;
    
    ret = oled_write_cmd(OLED_CMD_SET_PRECHARGE);          // 0xD9
    if (ret != ESP_OK) goto init_error;
    ret = oled_write_cmd(0xF1);                            // Recommended precharge period
    if (ret != ESP_OK) goto init_error;
    
    ret = oled_write_cmd(OLED_CMD_SET_VCOMH_DESELCT);      // 0xDB
    if (ret != ESP_OK) goto init_error;
    ret = oled_write_cmd(0x40);                            // VCOMH setting
    if (ret != ESP_OK) goto init_error;
    
    ret = oled_write_cmd(OLED_CMD_DISPLAY_RAM);            // 0xA4 Display RAM content
    if (ret != ESP_OK) goto init_error;
    
    ret = oled_write_cmd(OLED_CMD_DISPLAY_NORMAL);         // 0xA6 Non-inverted display
    if (ret != ESP_OK) goto init_error;
    
    ret = oled_write_cmd(OLED_CMD_DISPLAY_ON);             // 0xAF Turn on display
    if (ret != ESP_OK) goto init_error;
    
    // Brief delay after initialization to ensure display is ready to receive data
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Clear screen
    oled_clear_buffer();
    oled_refresh();
    
    // Test display - draw a frame to confirm display is working
    for (int i = 0; i < OLED_WIDTH; i++) {
        // Top and bottom borders
        oled_buffer[i] = 0xFF;  // First row
        oled_buffer[(OLED_PAGES-1) * OLED_WIDTH + i] = 0xFF;  // Last row
    }
    
    for (int i = 0; i < OLED_PAGES; i++) {
        // Left and right borders
        oled_buffer[i * OLED_WIDTH] = 0xFF;  // First column
        oled_buffer[i * OLED_WIDTH + OLED_WIDTH - 1] = 0xFF;  // Last column
    }
    
    oled_refresh();
    vTaskDelay(pdMS_TO_TICKS(500)); // Display frame for 0.5 seconds
    
    // Clear screen
    oled_clear_buffer();
    oled_refresh();
    
    // Display welcome message
    oled_draw_string(0, 0, "Smart Door Lock");
    oled_draw_string(0, 2, "System Starting...");
    oled_refresh();
    
    ESP_LOGI(TAG, "OLED initialization complete");
    return ESP_OK;
    
init_error:
    ESP_LOGE(TAG, "OLED initialization failed: %s", esp_err_to_name(ret));
    return ret;
}

/**
 * Clear the OLED buffer
 */
void oled_clear_buffer(void)
{
    memset(oled_buffer, 0, sizeof(oled_buffer));
}

/**
 * Clear a specific page (line) of the display
 */
void oled_clear_page(uint8_t page)
{
    if (page >= OLED_PAGES) {
        return;
    }
    
    memset(&oled_buffer[page * OLED_WIDTH], 0, OLED_WIDTH);
}

/**
 * Refresh the OLED display with the current buffer contents
 */
esp_err_t oled_refresh(void)
{
    esp_err_t ret;
    
    // Set column address range
    ret = oled_write_cmd(OLED_CMD_SET_COLUMN_RANGE);
    if (ret != ESP_OK) return ret;
    
    ret = oled_write_cmd(0);
    if (ret != ESP_OK) return ret;
    
    ret = oled_write_cmd(OLED_WIDTH - 1);
    if (ret != ESP_OK) return ret;
    
    // Set page address range
    ret = oled_write_cmd(OLED_CMD_SET_PAGE_RANGE);
    if (ret != ESP_OK) return ret;
    
    ret = oled_write_cmd(0);
    if (ret != ESP_OK) return ret;
    
    ret = oled_write_cmd(OLED_PAGES - 1);
    if (ret != ESP_OK) return ret;
    
    // Write display buffer in smaller chunks to ensure reliable transmission
    // Reduced chunk size for more stable I2C transmission
    const int chunk_size = OLED_CHUNK_SIZE;
    for (int i = 0; i < sizeof(oled_buffer); i += chunk_size) {
        int remaining = sizeof(oled_buffer) - i;
        int current_chunk = remaining > chunk_size ? chunk_size : remaining;
        ret = oled_write_data(&oled_buffer[i], current_chunk);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to write OLED buffer chunk at offset %d: %s", 
                     i, esp_err_to_name(ret));
            return ret;
        }
        // Give OLED time to process data
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    ESP_LOGD(TAG, "OLED refresh completed");
    return ESP_OK;
}

/**
 * Draw a character at the specified position
 */
void oled_draw_char(uint8_t x, uint8_t page, char c)
{
    if (c < 32 || c > 127) {
        c = '?'; // Replace non-printable characters
    }
    
    // Get character position in font table
    const uint8_t *char_ptr = font5x8[c - 32];
    
    // Calculate buffer position
    uint16_t buffer_pos = page * OLED_WIDTH + x;
    
    // Ensure we don't exceed buffer boundaries
    if (x > OLED_WIDTH - 6 || page >= OLED_PAGES) {
        return;
    }
    
    // Copy character data to buffer
    for (int i = 0; i < 5; i++) {
        oled_buffer[buffer_pos + i] = char_ptr[i];
    }
    
    // Add character spacing
    oled_buffer[buffer_pos + 5] = 0;
}

/**
 * Draw a string at the specified position
 */
void oled_draw_string(uint8_t x, uint8_t page, const char* str)
{
    uint8_t pos_x = x;
    while (*str != '\0') {
        if (pos_x > OLED_WIDTH - 6) {
            break; // Exceeds screen width
        }
        oled_draw_char(pos_x, page, *str);
        str++;
        pos_x += 6; // Character width 5 + spacing 1
    }
}

/**
 * Update OLED to show current door status
 */
void oled_update_status(door_state_t door_state, bool is_locked_out, 
                        TickType_t lockout_start_time, int password_index)
{
    if (!oled_take_mutex(100)) {
        ESP_LOGE(TAG, "Could not get OLED mutex for status update");
        return;
    }
    
    oled_clear_buffer();
    oled_draw_string(0, 0, "Smart Door Lock");
    
    if (door_state == DOOR_LOCKED) {
        oled_draw_string(0, 2, "Status: LOCKED");
    } else {
        oled_draw_string(0, 2, "Status: UNLOCKED");
    }
    
    if (is_locked_out) {
        TickType_t remaining = LOCKOUT_DURATION - (xTaskGetTickCount() - lockout_start_time);
        char lockout_msg[32];
        snprintf(lockout_msg, sizeof(lockout_msg), "Locked: %d sec", (int)(remaining / 1000));
        oled_draw_string(0, 4, lockout_msg);
    } else {
        oled_draw_string(0, 4, "Enter Pass/Scan Card");
        // Update password display (asterisks)
        char pwd_display[MAX_PASSWORD_LENGTH + 1] = {0};
        for (int i = 0; i < password_index; i++) {
            pwd_display[i] = '*';
        }
        oled_draw_string(0, 6, pwd_display);
    }
    
    oled_refresh();
    oled_give_mutex();
}

/**
 * Update password display on OLED
 */
void oled_update_password(int password_index)
{
    if (!oled_take_mutex(100)) {
        ESP_LOGE(TAG, "Could not get OLED mutex for password update");
        return;
    }
    
    // Clear password line first
    oled_clear_page(6);
    
    // Use asterisks to represent entered password
    char pwd_display[MAX_PASSWORD_LENGTH + 1] = {0};
    for (int i = 0; i < password_index; i++) {
        pwd_display[i] = '*';
    }
    
    oled_draw_string(0, 6, pwd_display);
    oled_refresh();
    
    oled_give_mutex();
}

/**
 * Show a message on OLED with optional delay
 */
void oled_show_message(const char* message, int delay_ms)
{
    if (!oled_take_mutex(100)) {
        ESP_LOGE(TAG, "Could not get OLED mutex for message");
        return;
    }
    
    // Clear message line
    oled_clear_page(6);
    
    oled_draw_string(0, 6, message);
    oled_refresh();
    
    oled_give_mutex();
    
    if (delay_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

/**
 * Display RFID UID on OLED
 */
void oled_show_uid(uint8_t *uid, uint8_t uid_size)
{
    if (!oled_take_mutex(100)) {
        ESP_LOGE(TAG, "Could not get OLED mutex for UID display");
        return;
    }
    
    char uid_str[32] = {0};
    char hex_str[4];
    
    strcpy(uid_str, "UID: ");
    for (int i = 0; i < uid_size; i++) {
        sprintf(hex_str, "%02X ", uid[i]);
        strcat(uid_str, hex_str);
    }
    
    // Clear UID line first to prevent overlap
    oled_clear_page(5);
    
    oled_draw_string(0, 5, uid_str);
    oled_refresh();
    
    oled_give_mutex();
}

/**
 * Take OLED mutex before display operations
 */
bool oled_take_mutex(uint32_t timeout_ms)
{
    if (oled_mutex == NULL) {
        ESP_LOGE(TAG, "OLED mutex not initialized");
        return false;
    }
    
    if (xSemaphoreTake(oled_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE) {
        return true;
    }
    
    return false;
}

/**
 * Give back OLED mutex after display operations
 */
void oled_give_mutex(void)
{
    if (oled_mutex == NULL) {
        ESP_LOGE(TAG, "OLED mutex not initialized");
        return;
    }
    
    xSemaphoreGive(oled_mutex);
}