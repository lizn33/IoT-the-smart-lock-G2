#include <stdio.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"
#include "led.h"

static const char *TAG = TAG_LOCK;

/**
 * Initialize LED
 */
esp_err_t led_init(void)
{
    ESP_LOGI(TAG, "Initializing LED...");
    
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << LED_PIN),
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LED GPIO configuration failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initially off
    gpio_set_level(LED_PIN, 0);
    
    ESP_LOGI(TAG, "LED initialization complete");
    return ESP_OK;
}

/**
 * Control LED state
 */
void led_set_state(bool on)
{
    gpio_set_level(LED_PIN, on ? 1 : 0);
    ESP_LOGD(TAG, "LED turned %s", on ? "ON" : "OFF");
}

/**
 * Flash the LED a specified number of times
 */
void led_flash(int times, int on_time, int off_time)
{
    for (int i = 0; i < times; i++) {
        led_set_state(true);
        vTaskDelay(pdMS_TO_TICKS(on_time));
        led_set_state(false);
        vTaskDelay(pdMS_TO_TICKS(off_time));
    }
}