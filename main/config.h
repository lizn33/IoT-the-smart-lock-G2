#ifndef CONFIG_H
#define CONFIG_H

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"

/* ===== System Tags ===== */
#define TAG_RFID "RFID_MODULE"
#define TAG_LOCK "SMART_LOCK"
#define TAG_OLED "OLED_DISPLAY"
#define TAG_KPD  "KEYPAD"

/* ===== RC522 RFID Module Configuration ===== */
// RC522 SPI Pins
#define RC522_SPI_HOST     SPI2_HOST
#define RC522_SPI_MISO     GPIO_NUM_4    // Data In (MISO)
#define RC522_SPI_MOSI     GPIO_NUM_5    // Data Out (MOSI)
#define RC522_SPI_SCLK     GPIO_NUM_6    // Clock (SCK)
#define RC522_SPI_CS       GPIO_NUM_16   // Chip Select (CS/SS)
#define RC522_RST          GPIO_NUM_17   // Reset pin

// RC522 Register Definitions
#define RC522_REG_COMMAND        0x01
#define RC522_REG_COM_IE_N       0x02
#define RC522_REG_DIV_IE_N       0x03
#define RC522_REG_COM_IRQ        0x04
#define RC522_REG_DIV_IRQ        0x05
#define RC522_REG_ERROR          0x06
#define RC522_REG_STATUS1        0x07
#define RC522_REG_STATUS2        0x08
#define RC522_REG_FIFO_DATA      0x09
#define RC522_REG_FIFO_LEVEL     0x0A
#define RC522_REG_WATER_LEVEL    0x0B
#define RC522_REG_CONTROL        0x0C
#define RC522_REG_BIT_FRAMING    0x0D
#define RC522_REG_COLL           0x0E
#define RC522_REG_MODE           0x11
#define RC522_REG_TX_MODE        0x12
#define RC522_REG_RX_MODE        0x13
#define RC522_REG_TX_CONTROL     0x14
#define RC522_REG_TX_ASK         0x15
#define RC522_REG_TX_SEL         0x16
#define RC522_REG_RX_SEL         0x17
#define RC522_REG_RX_THRESHOLD   0x18
#define RC522_REG_DEMOD          0x19
#define RC522_REG_MF_TX          0x1C
#define RC522_REG_MF_RX          0x1D
#define RC522_REG_SERIAL_SPEED   0x1F
#define RC522_REG_CRC_RESULT_H   0x21
#define RC522_REG_CRC_RESULT_L   0x22
#define RC522_REG_MOD_WIDTH      0x24
#define RC522_REG_RF_CFG         0x26
#define RC522_REG_GS_N           0x27
#define RC522_REG_CW_GS_P        0x28
#define RC522_REG_MOD_GS_P       0x29
#define RC522_REG_T_MODE         0x2A
#define RC522_REG_T_PRESCALER    0x2B
#define RC522_REG_T_RELOAD_H     0x2C
#define RC522_REG_T_RELOAD_L     0x2D
#define RC522_REG_T_COUNTER_H    0x2E
#define RC522_REG_T_COUNTER_L    0x2F
#define RC522_REG_TEST_SEL1      0x31
#define RC522_REG_TEST_SEL2      0x32
#define RC522_REG_TEST_PIN_EN    0x33
#define RC522_REG_TEST_PIN_VALUE 0x34
#define RC522_REG_TEST_BUS       0x35
#define RC522_REG_AUTO_TEST      0x36
#define RC522_REG_VERSION        0x37
#define RC522_REG_ANALOG_TEST    0x38
#define RC522_REG_TEST_ADC1      0x39
#define RC522_REG_TEST_ADC2      0x3A
#define RC522_REG_TEST_ADC0      0x3B
#define RC522_REG_RANDOM_ID      0x3C
#define RC522_REG_RANDOM_ID2     0x3D
#define RC522_REG_IO_CONFIG1     0x3E
#define RC522_REG_IO_CONFIG2     0x3F

// RC522 Commands
#define RC522_CMD_IDLE           0x00
#define RC522_CMD_MEM            0x01
#define RC522_CMD_GENERATE_RANDOM_ID 0x02
#define RC522_CMD_CALC_CRC       0x03
#define RC522_CMD_TRANSMIT       0x04
#define RC522_CMD_NO_CMD_CHANGE  0x07
#define RC522_CMD_RECEIVE        0x08
#define RC522_CMD_TRANSCEIVE     0x0C
#define RC522_CMD_MF_AUTHENT     0x0E
#define RC522_CMD_SOFT_RESET     0x0F

// Mifare 1K Card Commands
#define PICC_CMD_REQA           0x26
#define PICC_CMD_WUPA           0x52
#define PICC_CMD_ANTICOLL_1     0x93
#define PICC_CMD_ANTICOLL_2     0x95
#define PICC_CMD_ANTICOLL_3     0x97
#define PICC_CMD_SELECT_1       0x93
#define PICC_CMD_SELECT_2       0x95
#define PICC_CMD_SELECT_3       0x97
#define PICC_CMD_HALT           0x50

// Error Codes
#define RC522_OK                 0
#define RC522_ERR               -1
#define RC522_ERR_TIMEOUT       -2
#define RC522_ERR_CRC           -3
#define RC522_ERR_COLLISION     -4
#define RC522_ERR_BUFFER_OVFL   -5
#define RC522_ERR_PROTOCOL      -6
#define RC522_ERR_WRONG_UID_CK  -7
#define RC522_ERR_CRC_WRONG     -8
#define RC522_ERR_MIFARE_AUTH   -9

// Timeout
#define RC522_DEFAULT_TIMEOUT_MS 150  // Increased from 50ms to 150ms

/* ===== Smart Lock Configuration ===== */
// Pin Definitions
#define SERVO_PIN       GPIO_NUM_3    // Servo control
#define LED_PIN         GPIO_NUM_15   // Status LED

// OLED Display I2C pins
#define I2C_MASTER_SCL_IO          22
#define I2C_MASTER_SDA_IO          21
#define I2C_MASTER_NUM             I2C_NUM_0
#define I2C_MASTER_FREQ_HZ         100000    // Reduced from 400kHz to 100kHz for stability
#define I2C_MASTER_TIMEOUT_MS      200       // Increased timeout for I2C operations
#define I2C_MASTER_TX_BUF_DISABLE  0
#define I2C_MASTER_RX_BUF_DISABLE  0
#define OLED_ADDR                  0x3C

// OLED SSD1306 commands
#define OLED_CONTROL_BYTE_CMD_SINGLE    0x80
#define OLED_CONTROL_BYTE_CMD_STREAM    0x00
#define OLED_CONTROL_BYTE_DATA_STREAM   0x40

#define OLED_CMD_SET_CONTRAST           0x81
#define OLED_CMD_DISPLAY_RAM            0xA4
#define OLED_CMD_DISPLAY_ALLON          0xA5
#define OLED_CMD_DISPLAY_NORMAL         0xA6
#define OLED_CMD_DISPLAY_INVERTED       0xA7
#define OLED_CMD_DISPLAY_OFF            0xAE
#define OLED_CMD_DISPLAY_ON             0xAF

#define OLED_CMD_SET_MEMORY_ADDR_MODE   0x20
#define OLED_CMD_SET_COLUMN_RANGE       0x21
#define OLED_CMD_SET_PAGE_RANGE         0x22

#define OLED_CMD_SET_START_LINE         0x40
#define OLED_CMD_SET_SEGMENT_REMAP      0xA0
#define OLED_CMD_SET_MUX_RATIO          0xA8
#define OLED_CMD_SET_COM_SCAN_MODE      0xC0
#define OLED_CMD_SET_DISPLAY_OFFSET     0xD3
#define OLED_CMD_SET_COM_PIN_MAP        0xDA
#define OLED_CMD_NOP                    0xE3

#define OLED_CMD_SET_DISPLAY_CLK_DIV    0xD5
#define OLED_CMD_SET_PRECHARGE          0xD9
#define OLED_CMD_SET_VCOMH_DESELCT      0xDB

#define OLED_CMD_CHARGE_PUMP            0x8D

// OLED dimensions
#define OLED_WIDTH                      128
#define OLED_HEIGHT                     64
#define OLED_PAGES                      (OLED_HEIGHT / 8)
#define OLED_CHUNK_SIZE                 16      // Reduced chunk size for more stable I2C transmission
#define OLED_MAX_RETRY                  3       // Maximum retries for I2C operations

// Keypad pins 
#define ROW_NUM     4
#define COL_NUM     4
// Row pins (output)
#define ROW_1_PIN   GPIO_NUM_7  
#define ROW_2_PIN   GPIO_NUM_0  
#define ROW_3_PIN   GPIO_NUM_1  
#define ROW_4_PIN   GPIO_NUM_10 
// Column pins (input)
#define COL_1_PIN   GPIO_NUM_11 
#define COL_2_PIN   GPIO_NUM_2  
#define COL_3_PIN   GPIO_NUM_20 
#define COL_4_PIN   GPIO_NUM_19 

/* ===== Servo Configuration for Standard MS18 Servo ===== */
#define SERVO_TIMER             LEDC_TIMER_0
#define SERVO_MODE              LEDC_LOW_SPEED_MODE
#define SERVO_CHANNEL           LEDC_CHANNEL_0
#define SERVO_RESOLUTION_BITS   LEDC_TIMER_10_BIT
#define SERVO_FREQUENCY         50      // Standard 50Hz for servo control

/* Standard Servo Control Values (MS18) */
#define SERVO_MIN_PULSE         500     // Minimum pulse width (0 degrees) in microseconds
#define SERVO_MAX_PULSE         2500    // Maximum pulse width (180 degrees) in microseconds
#define SERVO_MID_PULSE         1500    // Middle position (90 degrees) in microseconds

/* Lock positions */
#define SERVO_LOCKED_POS        800     // Locked position pulse width in microseconds (~30 degrees)
#define SERVO_UNLOCKED_POS      2200    // Unlocked position pulse width in microseconds (~150 degrees)
#define SERVO_MOVE_TIME         1000    // Time in ms to allow the servo to reach position

// Password configuration
#define MAX_PASSWORD_LENGTH 10
#define MAX_ATTEMPTS 3              // Maximum attempts before lockout
#define LOCKOUT_DURATION 30000      // Lockout duration in ms (30 seconds)

// Authorized RFID UIDs
#define MAX_AUTHORIZED_CARDS 5
#define UID_LENGTH 4

/* Door State */
typedef enum {
    DOOR_LOCKED,
    DOOR_UNLOCKED
} door_state_t;

#endif /* CONFIG_H */