#ifndef RFID_H
#define RFID_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/spi_master.h"
#include "esp_err.h"
#include "config.h"

/**
 * @brief Initialize the RC522 RFID module
 * 
 * @return ESP_OK if initialization successful, otherwise error
 */
esp_err_t rfid_init(void);

/**
 * @brief Detect card and read UID
 * 
 * @param uid Buffer to store UID, at least UID_LENGTH bytes
 * @param uid_size Returns UID size (bytes)
 * @return true if card detected and UID read successfully, otherwise false
 */
bool rfid_read_card_uid(uint8_t *uid, uint8_t *uid_size);

/**
 * @brief Check if UID is in authorized list
 * 
 * @param uid Buffer containing UID
 * @param uid_size Size of UID in bytes
 * @return true if UID is authorized, otherwise false
 */
bool rfid_check_authorized_uid(uint8_t *uid, uint8_t uid_size);

/**
 * @brief Add UID to authorized list
 * 
 * @param uid Buffer containing UID
 * @param uid_size Size of UID in bytes
 * @return true if UID was added, false if list is full
 */
bool rfid_add_authorized_uid(uint8_t *uid, uint8_t uid_size);

/**
 * @brief RFID task for continuously scanning for cards
 * 
 * @param arg Task arguments (not used)
 */
void rfid_task(void *arg);

/**
 * @brief Initialize the list of authorized UIDs
 */
void rfid_init_authorized_list(void);

/**
 * @brief Get current authorized UID count
 * 
 * @return Number of authorized UIDs
 */
int rfid_get_authorized_count(void);

#endif /* RFID_H */