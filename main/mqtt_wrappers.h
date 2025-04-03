#ifndef MQTT_WRAPPERS_H
#define MQTT_WRAPPERS_H

#ifdef __cplusplus
extern "C" {
#endif

// Initialize WiFi (wrapper for WifiManager::init)
void wifi_init(const char* ssid, const char* password);

// Initialize MQTT (wrapper for MqttManager::init and MqttMessage::init)
void mqtt_init(const char* broker_uri, const char* client_id, 
               const char* username, const char* password);

void mqtt_send_lock_status(const char* device_id, bool is_locked);

void mqtt_send_lock_code(const char *device_id, const char *code_id, const char *code);

void mqtt_send_alert(const char *device_id, const char *alert_type);

void request_all_code(const char *device_id);

void handle_password_message(const char* device_id, const char* code_id, const char* password, const char* timestamp);

// Get device ID from MAC address
void init_device_id(char *out_device_id, int max_len);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_WRAPPERS_H */