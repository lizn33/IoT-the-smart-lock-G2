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
 * Convert angle to microseconds
 */
uint32_t servo_angle_to_microseconds(uint8_t angle)
{
    // Ensure angle is within 0-180 range
    if (angle > 180) angle = 180;
    
    // Linear mapping from angle (0-180) to pulse width (SERVO_MIN_PULSE - SERVO_MAX_PULSE)
    return SERVO_MIN_PULSE + (((SERVO_MAX_PULSE - SERVO_MIN_PULSE) * angle) / 180);
}

/**
 * Set servo to a specific angle (0-180 degrees)
 */
esp_err_t servo_set_angle(uint8_t angle)
{
    uint32_t pulse_width_us = servo_angle_to_microseconds(angle);
    ESP_LOGI(TAG, "Setting servo to angle: %u° (pulse: %u us)", (unsigned int)angle, (unsigned int)pulse_width_us);
    return servo_move(pulse_width_us);
}

/**
 * Set servo control signal in microseconds
 */
esp_err_t servo_move(uint32_t pulse_width_us)
{
    // Constrain pulse width to valid range
    if (pulse_width_us < SERVO_MIN_PULSE) pulse_width_us = SERVO_MIN_PULSE;
    if (pulse_width_us > SERVO_MAX_PULSE) pulse_width_us = SERVO_MAX_PULSE;
    
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
    
    ESP_LOGI(TAG, "Servo pulse set to: %u us (duty: %u)", (unsigned int)pulse_width_us, (unsigned int)duty);
    return ESP_OK;
}

/**
 * Stop the servo (prevent jittering)
 */
esp_err_t servo_stop(void)
{
    // For standard servos, we can either:
    // 1. Detach by disabling output (chosen method)
    // 2. Move to a resting position and then detach
    
    esp_err_t ret = ledc_stop(SERVO_MODE, SERVO_CHANNEL, 0);
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
    ESP_LOGI(TAG, "Initializing MS18 standard servo motor...");
    
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
        .duty           = servo_microseconds_to_duty(SERVO_MID_PULSE),  // Start at middle position
        .hpoint         = 0
    };
    
    ret = ledc_channel_config(&ledc_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Servo channel configuration failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initialize to locked position
    vTaskDelay(pdMS_TO_TICKS(100));
    ret = servo_move(SERVO_LOCKED_POS);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Allow time for servo to reach position, then detach to prevent jitter
    vTaskDelay(pdMS_TO_TICKS(SERVO_MOVE_TIME));
    ret = servo_stop();
    if (ret != ESP_OK) {
        return ret;
    }
    
    ESP_LOGI(TAG, "Servo initialization complete");
    return ESP_OK;
}

/**
 * Calibration routine for standard servo
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
    
    // Test minimum position (0 degrees)
    ESP_LOGI(TAG, "Testing minimum position: %d us", SERVO_MIN_PULSE);
    oled_draw_string(0, 2, "Min position (0 deg)");
    oled_refresh();
    ret = servo_move(SERVO_MIN_PULSE);
    if (ret != ESP_OK) goto calib_error;
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Test middle position (90 degrees)
    ESP_LOGI(TAG, "Testing middle position: %d us", SERVO_MID_PULSE);
    oled_draw_string(0, 4, "Mid position (90 deg)");
    oled_refresh();
    ret = servo_move(SERVO_MID_PULSE);
    if (ret != ESP_OK) goto calib_error;
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Test maximum position (180 degrees)
    ESP_LOGI(TAG, "Testing maximum position: %d us", SERVO_MAX_PULSE);
    oled_draw_string(0, 6, "Max position (180 deg)");
    oled_refresh();
    ret = servo_move(SERVO_MAX_PULSE);
    if (ret != ESP_OK) goto calib_error;
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Test locked position
    ESP_LOGI(TAG, "Testing locked position: %d us", SERVO_LOCKED_POS);
    oled_draw_string(0, 6, "Locked position");
    oled_refresh();
    ret = servo_move(SERVO_LOCKED_POS);
    if (ret != ESP_OK) goto calib_error;
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Test unlocked position
    ESP_LOGI(TAG, "Testing unlocked position: %d us", SERVO_UNLOCKED_POS);
    oled_draw_string(0, 6, "Unlocked position");
    oled_refresh();
    ret = servo_move(SERVO_UNLOCKED_POS);
    if (ret != ESP_OK) goto calib_error;
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Return to locked position and detach
    ret = servo_move(SERVO_LOCKED_POS);
    if (ret != ESP_OK) goto calib_error;
    vTaskDelay(pdMS_TO_TICKS(SERVO_MOVE_TIME));
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