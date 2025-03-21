#include "cloudComm/mqtt_manager.h"
#include "cloudComm/wifi_manager.h"
#include "cloudComm/mqtt_message.h"

// 确保 app_main 具有 C 链接性
extern "C" void app_main(void)
{
    const char *broker_username = "alice";
    const char *broker_password = "alice123456";
    const char *broker_uri = "mqtt://52.169.3.167:1883";
    const char *client_id = "esp32_client_01";

    const char *wifi_ssid = "Galaxy S24 Ultra AD23";
    const char *wifi_password = "qwezxc123";

    // WifiManager::init("Galaxy S24 Ultra AD23", "qwezxc123");
    // MqttManager::init("mqtt://192.109.228.41:1883", "esp32_client_01", "alice", "alice123456");
    WifiManager::init(wifi_ssid, wifi_password);
    MqttManager::init(broker_uri, client_id, broker_username, broker_password);
    MqttMessage::init();
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        MqttMessage::requestDeviceAllCode("10001");
    }
}