#include "mqtt_wrappers.h"
#include "cloudComm/mqtt_message.h"
#include "cloudComm/mqtt_manager.h"
#include "cloudComm/wifi_manager.h"
#include "esp_mac.h"
#include <string>

extern "C" {

void wifi_init(const char* ssid, const char* password) {
    WifiManager::init(ssid, password);
}

void mqtt_init(const char* broker_uri, const char* client_id, 
               const char* username, const char* password) {
    MqttManager::init(broker_uri, client_id, username, password);
    MqttMessage::init();
}

void mqtt_send_lock_status(const char* device_id, bool is_locked) {
    MqttMessage::sendDeviceLock(std::string(device_id), is_locked);
}

void mqtt_send_lock_code(const char* device_id, const char* code_id, const char* code) {
    MqttMessage::sendDeviceLockCode(std::string(device_id), std::string(code_id), std::string(code));
}

void mqtt_send_alert(const char* device_id, const char* alert_type) {
    MqttMessage::sendDeviceLockAlert(std::string(device_id), std::string(alert_type));
}

void request_all_code(const char *device_id)
{
    MqttMessage::requestDeviceAllCode(std::string(device_id));
}

void init_device_id(char* out_device_id, int max_len) {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(out_device_id, max_len, "%02X%02X%02X%02X%02X%02X", 
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

}