#include <stdio.h>
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"
#include "servo.h"
#include "oled.h"

static const char *TAG = TAG_LOCK;

/**
 * Convert microseconds to LEDC duty cycle value
 */
uint32_t servo_microseconds_to_duty(uint32_t us)
{
    // Calculate duty cycle based on pulse width
    // duty = (2^resolution) * (pulse_width / period)
    // where period = 1/frequency = 1/50Hz = 20ms = 20000us
    uint32_t max_duty = (1 << SERVO_RESOLUTION_BITS) - 1;
    return (us * max_duty) / 20000;
}

/**
 * Set servo control signal in microseconds
 */
esp_err_t servo_move(uint32_t pulse_width_us)
{
    uint32_t duty = servo_microseconds_to_duty(pulse_width_us);
    
    esp_err_t ret = ledc_set_duty(SERVO_MODE, SERVO_CHANNEL, duty);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Setting servo duty failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = ledc_update_duty(SERVO_MODE, SERVO_CHANNEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Updating servo duty failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // ESP_LOGI(TAG, "Servo pulse set to: %d us (duty: %d)", pulse_width_us, duty);
    ESP_LOGI(TAG, "Servo pulse set to: %u us (duty: %u)", (unsigned int)pulse_width_us, (unsigned int)duty);
    return ESP_OK;
}

/**
 * Stop the servo (prevent jittering)
 */
esp_err_t servo_stop(void)
{
    // Set to stop position first
    esp_err_t ret = servo_move(SERVO_STOP_PULSE);
    if (ret != ESP_OK) {
        return ret;
    }
    
    vTaskDelay(pdMS_TO_TICKS(100)); // Brief delay to ensure command is received
    
    // Detach servo by disabling output
    ret = ledc_stop(SERVO_MODE, SERVO_CHANNEL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Stopping servo failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Servo stopped and detached");
    return ESP_OK;
}

/**
 * Initialize the LEDC peripheral for servo control
 */
esp_err_t servo_init(void)
{
    ESP_LOGI(TAG, "Initializing servo motor...");
    
    // Configure LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = SERVO_MODE,
        .timer_num        = SERVO_TIMER,
        .duty_resolution  = SERVO_RESOLUTION_BITS,
        .freq_hz          = SERVO_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    
    esp_err_t ret = ledc_timer_config(&ledc_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Servo timer configuration failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure LEDC channel
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = SERVO_MODE,
        .channel        = SERVO_CHANNEL,
        .timer_sel      = SERVO_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = SERVO_PIN,
        .duty           = servo_microseconds_to_duty(SERVO_STOP_PULSE),  // Start at stop position
        .hpoint         = 0
    };
    
    ret = ledc_channel_config(&ledc_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Servo channel configuration failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Ensure servo is stopped
    vTaskDelay(pdMS_TO_TICKS(100));
    ret = servo_stop();
    if (ret != ESP_OK) {
        return ret;
    }
    
    ESP_LOGI(TAG, "Servo initialization complete");
    return ESP_OK;
}

/**
 * Calibration routine for continuous rotation servo
 */
esp_err_t servo_calibrate(void)
{
    ESP_LOGI(TAG, "Starting servo calibration sequence...");
    esp_err_t ret;
    
    // Take OLED mutex for display updates
    if (!oled_take_mutex(100)) {
        ESP_LOGE(TAG, "Could not get OLED mutex for servo calibration");
        return ESP_FAIL;
    }
    
    // Update OLED display
    oled_clear_buffer();
    oled_draw_string(0, 0, "Servo Calibration");
    oled_refresh();
    
    // Test stop position
    ESP_LOGI(TAG, "Testing stop position: %d us", SERVO_STOP_PULSE);
    oled_draw_string(0, 2, "Stop position");
    oled_refresh();
    ret = servo_move(SERVO_STOP_PULSE);
    if (ret != ESP_OK) goto calib_error;
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Test clockwise rotation (Lock direction)
    ESP_LOGI(TAG, "Testing CW rotation: %d us", SERVO_CW_SPEED);
    oled_draw_string(0, 4, "CW rotation (Lock)");
    oled_refresh();
    ret = servo_move(SERVO_CW_SPEED);
    if (ret != ESP_OK) goto calib_error;
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Return to stop
    ret = servo_move(SERVO_STOP_PULSE);
    if (ret != ESP_OK) goto calib_error;
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Test counter-clockwise rotation (Unlock direction)
    ESP_LOGI(TAG, "Testing CCW rotation: %d us", SERVO_CCW_SPEED);
    oled_draw_string(0, 6, "CCW rotation (Unlock)");
    oled_refresh();
    ret = servo_move(SERVO_CCW_SPEED);
    if (ret != ESP_OK) goto calib_error;
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Return to stop and detach
    ret = servo_move(SERVO_STOP_PULSE);
    if (ret != ESP_OK) goto calib_error;
    vTaskDelay(pdMS_TO_TICKS(1000));
    ret = servo_stop();
    if (ret != ESP_OK) goto calib_error;
    
    ESP_LOGI(TAG, "Servo calibration complete");
    oled_draw_string(0, 7, "Calibration done");
    oled_refresh();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Give back the mutex
    oled_give_mutex();
    return ESP_OK;
    
calib_error:
    ESP_LOGE(TAG, "Servo calibration failed: %s", esp_err_to_name(ret));
    oled_draw_string(0, 7, "Calibration failed!");
    oled_refresh();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Give back the mutex
    oled_give_mutex();
    return ret;
}