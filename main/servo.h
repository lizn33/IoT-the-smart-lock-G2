#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/ledc.h"
#include "config.h"

/**
 * @brief Initialize the servo motor
 * 
 * @return ESP_OK if initialization successful, otherwise error
 */
esp_err_t servo_init(void);

/**
 * @brief Set servo control signal in microseconds
 * 
 * @param pulse_width_us Pulse width in microseconds (1000-2000 typical range)
 * @return ESP_OK if successful, otherwise error
 */
esp_err_t servo_move(uint32_t pulse_width_us);

/**
 * @brief Stop the servo (prevent jittering)
 * 
 * @return ESP_OK if successful, otherwise error
 */
esp_err_t servo_stop(void);

/**
 * @brief Perform a calibration routine for servo
 * 
 * @return ESP_OK if successful, otherwise error
 */
esp_err_t servo_calibrate(void);

/**
 * @brief Convert microseconds to LEDC duty cycle
 * 
 * @param us Pulse width in microseconds
 * @return Corresponding duty cycle value
 */
uint32_t servo_microseconds_to_duty(uint32_t us);

#endif /* SERVO_H */