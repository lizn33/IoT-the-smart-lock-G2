#include <string.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"
#include "keypad.h"
#include "oled.h"

static const char *TAG = TAG_KPD;

// Keypad layout
static const char keypad_keys[ROW_NUM][COL_NUM] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

// Row and column pin arrays
static const gpio_num_t row_pins[ROW_NUM] = {
    ROW_1_PIN, ROW_2_PIN, ROW_3_PIN, ROW_4_PIN
};

static const gpio_num_t col_pins[COL_NUM] = {
    COL_1_PIN, COL_2_PIN, COL_3_PIN, COL_4_PIN
};

/**
 * Initialize keypad
 */
esp_err_t keypad_init(void)
{
    ESP_LOGI(TAG, "Initializing keypad...");

    // Configure row pins as outputs
    gpio_config_t row_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 0,
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    
    // Configure column pins as inputs with pull-up resistors
    gpio_config_t col_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 0,
        .pull_down_en = 0,
        .pull_up_en = 1  // Enable internal pull-up resistors
    };
    
    // Set row pin bitmask
    for (int i = 0; i < ROW_NUM; i++) {
        row_conf.pin_bit_mask |= (1ULL << row_pins[i]);
    }
    
    // Set column pin bitmask
    for (int i = 0; i < COL_NUM; i++) {
        col_conf.pin_bit_mask |= (1ULL << col_pins[i]);
    }
    
    // Apply configuration
    esp_err_t ret = gpio_config(&row_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure row pins: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = gpio_config(&col_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure column pins: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initialize row pins to high (inactive state)
    for (int i = 0; i < ROW_NUM; i++) {
        gpio_set_level(row_pins[i], 1);
    }
    
    ESP_LOGI(TAG, "Keypad initialization complete");
    return ESP_OK;
}

/**
 * Scan keypad for pressed keys
 */
char keypad_scan(void)
{
    char pressed_key = 0;
    
    // Scan each row
    for (int row = 0; row < ROW_NUM; row++) {
        // Set current row to low (active)
        gpio_set_level(row_pins[row], 0);
        
        // Give circuit time to stabilize
        vTaskDelay(pdMS_TO_TICKS(5));
        
        // Check each column
        for (int col = 0; col < COL_NUM; col++) {
            // If column is low, key is pressed
            if (gpio_get_level(col_pins[col]) == 0) {
                pressed_key = keypad_keys[row][col];
                
                // Wait for key release (simple debounce)
                while (gpio_get_level(col_pins[col]) == 0) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                
                // Extra delay to avoid key bounce
                vTaskDelay(pdMS_TO_TICKS(50));
            }
        }
        
        // Set current row back to high (inactive)
        gpio_set_level(row_pins[row], 1);
        
        // If key was found, stop scanning
        if (pressed_key != 0) {
            break;
        }
    }
    
    return pressed_key;
}

/**
 * Check if system is currently in lockout state
 */
bool keypad_check_lockout(TickType_t lockout_start_time)
{
    TickType_t now = xTaskGetTickCount();
    return ((now - lockout_start_time) > LOCKOUT_DURATION);
}

/**
 * Keypad task to handle password entry
 */
void keypad_task(void *arg)
{
    ESP_LOGI(TAG, "Keypad task started");
    
    // Process key input callback function from arg
    void (*process_key_input)(char) = (void (*)(char))arg;
    
    if (process_key_input == NULL) {
        ESP_LOGE(TAG, "Keypad task received NULL callback, stopping task");
        vTaskDelete(NULL);
        return;
    }
    
    while (1) {
        // Scan keypad
        char key = keypad_scan();
        
        // If key pressed, process input using callback
        if (key != 0) {
            process_key_input(key);
        }
        
        // Short delay to avoid CPU overuse
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}