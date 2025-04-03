#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

#include "config.h"
#include "oled.h"
#include "rfid.h"
#include "keypad.h"
#include "servo.h"
#include "led.h"

#include "mqtt_wrappers.h"

static const char *TAG = TAG_LOCK;
static const char* device_id = "10001";


// Password and door state
static char correct_password[MAX_PASSWORD_LENGTH] = "1234";  // Default password
static char entered_password[MAX_PASSWORD_LENGTH];           // User entered password
static int password_index = 0;                              // Password input position
static int attempt_count = 0;                               // Current attempt count
static bool is_locked_out = false;                          // Lockout status
static TickType_t lockout_start_time = 0;                   // Lockout start time
static door_state_t current_door_state = DOOR_LOCKED;       // Current door state

// Function declarations
void unlock_door(void);
void lock_door(void);
void process_key_input(char key);

/**
 * Lock the door
 */
void lock_door(void)
{
    if (current_door_state == DOOR_UNLOCKED) {
        ESP_LOGI(TAG, "Locking door...");
        
        // Update OLED display
        oled_show_message("Locking door...", 0);
        
        // Lock with servo
        servo_move(SERVO_CW_SPEED);
        vTaskDelay(pdMS_TO_TICKS(SERVO_ROTATION_TIME));
        servo_stop();
        
        // Update status
        led_set_state(false);  // Turn off LED when door is locked
        current_door_state = DOOR_LOCKED;
        
        // Update OLED status
        oled_update_status(current_door_state, is_locked_out, lockout_start_time, password_index);
        
        ESP_LOGI(TAG, "Door locked");
        mqtt_send_lock_status(device_id, true );

    } else {
        ESP_LOGI(TAG, "Door already locked");
    }
}

/**
 * Unlock the door
 */
void unlock_door(void)
{
    if (current_door_state == DOOR_LOCKED) {
        ESP_LOGI(TAG, "Unlocking door...");
        
        // Update OLED display
        oled_show_message("Unlocking door...", 0);
        
        // Unlock with servo
        servo_move(SERVO_CCW_SPEED);
        vTaskDelay(pdMS_TO_TICKS(SERVO_ROTATION_TIME));
        servo_stop();
        
        // Update status
        led_set_state(true);  // Turn on LED when door is unlocked
        current_door_state = DOOR_UNLOCKED;
        
        // Update OLED status
        oled_update_status(current_door_state, is_locked_out, lockout_start_time, password_index);
        
        ESP_LOGI(TAG, "Door unlocked");
        mqtt_send_lock_status(device_id, false);
    } else {
        ESP_LOGI(TAG, "Door already unlocked");
    }
}

/**
 * Clear entered password
 */
static void clear_password(void)
{
    password_index = 0;
    memset(entered_password, 0, MAX_PASSWORD_LENGTH);
    ESP_LOGI(TAG, "Password cleared");
    
    // Update OLED display
    oled_update_password(password_index);
    oled_show_message("Password cleared", 1000);
    oled_update_password(password_index); // Clear message
}

/**
 * Check password and unlock door if correct
 */
static void check_password(void)
{
    if (strcmp(entered_password, correct_password) == 0) {
        ESP_LOGI(TAG, "Password correct!");
        
        // Update OLED display
        oled_show_message("Password correct!", 1000);
        
        // Flash LED for visual feedback
        for (int i = 0; i < 3; i++) {
            led_set_state(true);
            vTaskDelay(pdMS_TO_TICKS(100));
            led_set_state(false);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        
        // Password correct, unlock door
        unlock_door();
        
        // Auto-lock after 5 seconds
        oled_show_message("Auto-lock in 5s", 0);
        vTaskDelay(pdMS_TO_TICKS(5000));
        lock_door();
        
        // Reset attempt counter
        attempt_count = 0;
    } else {
        ESP_LOGI(TAG, "Password incorrect!");
        mqtt_send_alert(device_id, "WRONG_PASSCODE");
        
        // Update OLED display
        char msg[32];
        snprintf(msg, sizeof(msg), "Wrong! (%d/%d)", attempt_count + 1, MAX_ATTEMPTS);
        oled_show_message(msg, 1500);
        
        // Error feedback - rapid LED flashing
        for (int i = 0; i < 5; i++) {
            led_set_state(true);
            vTaskDelay(pdMS_TO_TICKS(50));
            led_set_state(false);
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        
        // Increment attempt counter
        attempt_count++;
        
        // Check if maximum attempts reached
        if (attempt_count >= MAX_ATTEMPTS) {
            ESP_LOGI(TAG, "Maximum password attempts reached. System locked for %d seconds", LOCKOUT_DURATION / 1000);
            is_locked_out = true;
            lockout_start_time = xTaskGetTickCount();
            
            // Update OLED display
            oled_show_message("System locked!", 1500);
            oled_update_status(current_door_state, is_locked_out, lockout_start_time, password_index);
            mqtt_send_alert(device_id, "LOCKOUT");
            
            // Long LED flashing to indicate lockout
            for (int i = 0; i < 10; i++) {
                led_set_state(true);
                vTaskDelay(pdMS_TO_TICKS(200));
                led_set_state(false);
                vTaskDelay(pdMS_TO_TICKS(200));
            }
        } else {
            // Update OLED display, ready for new password entry
            oled_update_status(current_door_state, is_locked_out, lockout_start_time, password_index);
        }
    }
    
    // Clear entered password
    clear_password();
}

/**
 * Process key input from keypad
 */
void process_key_input(char key)
{
    // Check lockout status
    if (is_locked_out) {
        if ((xTaskGetTickCount() - lockout_start_time) > LOCKOUT_DURATION) {
            is_locked_out = false;
            attempt_count = 0;
            ESP_LOGI(TAG, "Lockout period ended");
            // Update OLED display
            oled_update_status(current_door_state, is_locked_out, lockout_start_time, password_index);
        } else {
            return; // Still in lockout period, don't process keys
        }
    }
    
    ESP_LOGI(TAG, "Key pressed: %c", key);
    
    if (key == '#') {
        // '#' key as confirmation
        if (password_index > 0) {
            ESP_LOGI(TAG, "Checking password...");
            check_password();
        }
    } 
    else if (key == '*') {
        // '*' key as clear
        clear_password();
    } 
    else if (password_index < (MAX_PASSWORD_LENGTH - 1)) { // Limit password length, leave room for null terminator
        // Add key to password
        entered_password[password_index++] = key;
        entered_password[password_index] = '\0'; // Ensure null-terminated
        
        // Update OLED password display
        oled_update_password(password_index);
        
        // Briefly light LED as key feedback
        led_set_state(true);
        vTaskDelay(pdMS_TO_TICKS(50));
        led_set_state(false);
    }
}

/**
 * Main function
 */
void app_main(void)
{
    // Set log levels to ensure detailed information
    esp_log_level_set(TAG_LOCK, ESP_LOG_INFO);
    esp_log_level_set(TAG_RFID, ESP_LOG_INFO);
    esp_log_level_set(TAG_OLED, ESP_LOG_INFO);
    esp_log_level_set(TAG_KPD, ESP_LOG_INFO);
    esp_log_level_set("*", ESP_LOG_INFO);  // Set log level for all modules
    
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Smart Door Lock System Starting...");
    ESP_LOGI(TAG, "=================================================");
    
    // Brief delay to ensure system stability
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Initialize most basic components first
    ESP_LOGI(TAG, "Initializing LED...");
    led_init();
    
    // LED flashing indicates system startup
    for (int i = 0; i < 3; i++) {
        led_set_state(true);
        vTaskDelay(pdMS_TO_TICKS(100));
        led_set_state(false);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Initialize I2C and OLED display
    ESP_LOGI(TAG, "Initializing OLED display...");
    esp_err_t ret = oled_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OLED initialization failed: %s", esp_err_to_name(ret));
        // Continue without OLED
    }
    
    // Initialize servo motor
    ESP_LOGI(TAG, "Initializing servo motor...");
    servo_init();
    
    // Initialize keypad
    ESP_LOGI(TAG, "Initializing keypad...");
    keypad_init();
    
    // Initialize RFID module with increased timeout
    ESP_LOGI(TAG, "Initializing RFID module...");
    esp_err_t rfid_status = rfid_init();
    if(rfid_status != ESP_OK) {
        ESP_LOGE(TAG, "RC522 initialization failed: %s", esp_err_to_name(rfid_status));
        oled_show_message("RFID Init Failed!", 2000);
    } else {
        oled_show_message("RFID Ready!", 1000);
    }
    
    // Initialize device ID
    ESP_LOGI(TAG, "Device ID: %s", device_id);
    
    // WiFi credentials
    const char *wifi_ssid = "Galaxy S24 Ultra AD23"; // Use your actual WiFi
    const char *wifi_password = "qwezxc123";

    // MQTT credentials
    const char* broker_uri = "mqtts://emqx.8117.me:8883";
    const char* client_id = "esp32_door_lock";
    const char* broker_username = "alice";
    const char* broker_password = "alice123456";
    
    // Initialize WiFi and wait for connection
    ESP_LOGI(TAG, "Initializing WiFi...");
    wifi_init(wifi_ssid, wifi_password);
    vTaskDelay(pdMS_TO_TICKS(15000)); // Wait for WiFi connection
    
    // Initialize MQTT
    ESP_LOGI(TAG, "Initializing MQTT...");
    mqtt_init(broker_uri, client_id, broker_username, broker_password);
    vTaskDelay(pdMS_TO_TICKS(2000)); // Wait for MQTT connection

    // Ensure door is in locked position at startup
    oled_show_message("Locking door...", 0);
    lock_door();
    
    // Create task arguments - callback functions array
    void (*rfid_callbacks[2])(void) = {unlock_door, lock_door};  // Fixed array of function pointers
    
    // Create keypad task arguments - callback function
    void *keypad_task_arg = (void*)process_key_input;
    
    // Create RFID scanning task
    ESP_LOGI(TAG, "Starting RFID scanning task...");
    TaskHandle_t rfid_task_handle = NULL;
    if (xTaskCreate(rfid_task, "rfid_task", 4096, rfid_callbacks, 5, &rfid_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create RFID task!");
    }
    
    // Create keypad handling task
    ESP_LOGI(TAG, "Starting keypad scanning task...");
    TaskHandle_t keypad_task_handle = NULL;
    if (xTaskCreate(keypad_task, "keypad_task", 2048, keypad_task_arg, 5, &keypad_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create keypad task!");
    }
    
    // Show final initialization completion screen
    oled_show_message("System Ready!", 1000);
    oled_update_status(current_door_state, is_locked_out, lockout_start_time, password_index);
    
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "System initialization complete.");
    ESP_LOGI(TAG, "Enter password or scan authorized RFID card to unlock door.");
    ESP_LOGI(TAG, "=================================================");

    mqtt_send_lock_status("10001", true);
    vTaskDelay(pdMS_TO_TICKS(1000));
    mqtt_send_lock_code("10001", "100", "100");
    vTaskDelay(pdMS_TO_TICKS(1000));
    mqtt_send_alert("10001", "MOTOR");
    vTaskDelay(pdMS_TO_TICKS(1000));
    request_all_code("10001");
    vTaskDelay(pdMS_TO_TICKS(1000));
}