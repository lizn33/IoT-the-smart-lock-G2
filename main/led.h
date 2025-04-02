#ifndef LED_H
#define LED_H

#include <stdbool.h>
#include "esp_err.h"
#include "config.h"

/**
 * @brief Initialize the LED GPIO
 * 
 * @return ESP_OK if initialization successful, otherwise error
 */
esp_err_t led_init(void);

/**
 * @brief Set the LED state
 * 
 * @param on true to turn LED on, false to turn off
 */
void led_set_state(bool on);

/**
 * @brief Flash the LED a specified number of times
 * 
 * @param times Number of flashes
 * @param on_time On duration in milliseconds
 * @param off_time Off duration in milliseconds
 */
void led_flash(int times, int on_time, int off_time);

#endif /* LED_H */