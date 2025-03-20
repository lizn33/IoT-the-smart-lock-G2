#ifndef CLOUD_CON_H
#define CLOUD_CON_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // Wi-Fi 事件组
    extern EventGroupHandle_t wifi_event_group;
    extern const int WIFI_CONNECTED_BIT;

    // 初始化 Wi-Fi
    void wifi_init(void);

    // 初始化 MQTT
    void mqtt_init(void);

#ifdef __cplusplus
}
#endif

#endif // CLOUD_CON_H
