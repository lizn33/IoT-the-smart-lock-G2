#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "nvs_flash.h"

static const char *TAG = "MQTT_EXAMPLE";

static void wifi_init(void)
{
    ESP_LOGI(TAG, "Initializing Wi-Fi...");

    // 初始化 NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // 初始化网络接口
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 初始化 Wi-Fi
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 配置 Wi-Fi STA 模式
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "Galaxy S24 Ultra AD23",
            .password = "qwezxc123",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(TAG, "Wi-Fi initialized successfully.");
}

static void mqtt_event_handler_cb(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        esp_mqtt_client_subscribe(event->client, "/topic/qos0", 0);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA, topic=%.*s, data=%.*s",
                 event->topic_len, event->topic,
                 event->data_len, event->data);
        break;
    default:
        break;
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Hello World!");

    // 初始化 Wi-Fi
    wifi_init();

    // 等待 Wi-Fi 连接成功
    ESP_LOGI(TAG, "Waiting for Wi-Fi connection...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://185.211.6.7:1883",
        .session.keepalive = 60,
        .network.disable_auto_reconnect = false, // 允许自动重连
        .session.protocol_ver = MQTT_PROTOCOL_V_3_1_1,
        .credentials.client_id = "esp32_client_01",
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler_cb, NULL);
    esp_mqtt_client_start(client);

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
