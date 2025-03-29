#ifndef OLED_H
#define OLED_H

#include <string.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "config.h"

/**
 * @brief Initialize the OLED display
 * 
 * @return ESP_OK if successful, otherwise error
 */
esp_err_t oled_init(void);

/**
 * @brief Clear the OLED buffer
 */
void oled_clear_buffer(void);

/**
 * @brief Refresh the OLED display with the current buffer contents
 * 
 * @return ESP_OK if successful, otherwise error
 */
esp_err_t oled_refresh(void);

/**
 * @brief Draw a character at the specified position
 * 
 * @param x X position (0-127)
 * @param page Page number (0-7)
 * @param c Character to draw
 */
void oled_draw_char(uint8_t x, uint8_t page, char c);

/**
 * @brief Draw a string at the specified position
 * 
 * @param x X position (0-127)
 * @param page Page number (0-7)
 * @param str String to draw
 */
void oled_draw_string(uint8_t x, uint8_t page, const char* str);

/**
 * @brief Update the display to show current door status
 */
void oled_update_status(door_state_t door_state, bool is_locked_out, 
                        TickType_t lockout_start_time, int password_index);

/**
 * @brief Update password display on OLED
 * 
 * @param password_index Current index in the password
 */
void oled_update_password(int password_index);

/**
 * @brief Show a message on OLED with optional delay
 * 
 * @param message Message to display
 * @param delay_ms Delay in milliseconds after displaying (0 for no delay)
 */
void oled_show_message(const char* message, int delay_ms);

/**
 * @brief Display RFID UID on OLED
 * 
 * @param uid Buffer containing UID bytes
 * @param uid_size Size of UID in bytes
 */
void oled_show_uid(uint8_t *uid, uint8_t uid_size);

/**
 * @brief Clear a specific page (line) of the display
 * 
 * @param page Page number (0-7)
 */
void oled_clear_page(uint8_t page);

/**
 * @brief Take OLED mutex before display operations
 * 
 * @param timeout_ms Timeout in milliseconds to wait for mutex
 * @return true if mutex was taken, false otherwise
 */
bool oled_take_mutex(uint32_t timeout_ms);

/**
 * @brief Give back OLED mutex after display operations
 */
void oled_give_mutex(void);

#endif /* OLED_H */