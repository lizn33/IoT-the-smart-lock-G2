#include "cloudComm/mqtt_manager.h"
#include "cloudComm/wifi_manager.h"
#include "cloudComm/mqtt_message.h"

// 确保 app_main 具有 C 链接性
extern "C" void mqtt_test_main(void)
{
    esp_log_level_set("*", ESP_LOG_DEBUG);

    const char *broker_username = "alice";
    const char *broker_password = "alice123456";
    const char *broker_uri = "mqtts://emqx.8117.me:8883";
    // const char *broker_uri = "mqtt://52.169.3.167:1883";
    const char *client_id = "esp32_client_01";

    const char *wifi_ssid = "test";
    const char *wifi_password = "12345678";

    // WifiManager::init("Galaxy S24 Ultra AD23", "qwezxc123");
    // MqttManager::init("mqtt://192.109.228.41:1883", "esp32_client_01", "alice", "alice123456");
    WifiManager::init(wifi_ssid, wifi_password);

    ESP_LOGI("app_main", "Waiting 30 seconds for Wi-Fi to connect...");
    vTaskDelay(pdMS_TO_TICKS(15000)); // 等 15 秒

    MqttManager::init(broker_uri, client_id, broker_username, broker_password);
    MqttMessage::init();
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(5000));

        // MqttMessage::sendDeviceLock("10001", true);
        // vTaskDelay(pdMS_TO_TICKS(1000));
        // MqttMessage::sendDeviceLockAlert("10001", "MOTOR");
        // vTaskDelay(pdMS_TO_TICKS(1000));
        // MqttMessage::requestDeviceAllCode("10001");

    }
}