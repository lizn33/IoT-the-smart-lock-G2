#include "mqtt_wrappers.h"
#include "cloudComm/mqtt_message.h"
#include "cloudComm/mqtt_manager.h"
#include "cloudComm/wifi_manager.h"
#include "esp_mac.h"
#include "mqtt_common.h"

extern QueueHandle_t mqtt_queue;

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

void handle_password_message(const char* device_id, const char* code_id, 
    const char* password, const char* timestamp) {
mqtt_event_t event;
event.type = MQTT_EVENT_PASSWORD;
strncpy(event.device_id, device_id, sizeof(event.device_id) - 1);
strncpy(event.password_data.code_id, code_id, sizeof(event.password_data.code_id) - 1);
strncpy(event.password_data.password, password, sizeof(event.password_data.password) - 1);
strncpy(event.password_data.timestamp, timestamp, sizeof(event.password_data.timestamp) - 1);

xQueueSend(mqtt_queue, &event, portMAX_DELAY);
}

void init_device_id(char* out_device_id, int max_len) {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(out_device_id, max_len, "%02X%02X%02X%02X%02X%02X", 
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

}