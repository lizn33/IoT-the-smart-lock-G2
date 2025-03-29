#ifndef KEYPAD_H
#define KEYPAD_H

#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"

/**
 * @brief Initialize the keypad hardware
 * 
 * @return ESP_OK if initialization successful, otherwise error
 */
esp_err_t keypad_init(void);

/**
 * @brief Scan keypad for pressed keys
 * 
 * @return Pressed key character, or 0 if no key is pressed
 */
char keypad_scan(void);

/**
 * @brief Keypad scanning task
 * 
 * @param arg Function pointer to keypad input processor
 */
void keypad_task(void *arg);

/**
 * @brief Check if system is currently in lockout state
 * 
 * @param lockout_start_time Timestamp when lockout started
 * @return true if lockout period has ended, false if still in lockout
 */
bool keypad_check_lockout(TickType_t lockout_start_time);

#endif /* KEYPAD_H */