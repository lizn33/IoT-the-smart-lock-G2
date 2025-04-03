// mqtt_common.h
#ifndef MQTT_COMMON_H
#define MQTT_COMMON_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "config.h"  // For MAX_PASSWORD_LENGTH

typedef enum {
    MQTT_EVENT_LOCK_STATUS,
    MQTT_EVENT_ALERT,
    MQTT_EVENT_PASSWORD
} mqtt_event_type_t;

typedef struct {
    mqtt_event_type_t type;
    char device_id[16];
    union {
        bool lock_status;
        char alert_message[32];
        struct {
            char code_id[16];
            char password[MAX_PASSWORD_LENGTH];
            char timestamp[32];
        } password_data;
    };
} mqtt_event_t;

extern QueueHandle_t mqtt_queue;

#endif /* MQTT_COMMON_H */