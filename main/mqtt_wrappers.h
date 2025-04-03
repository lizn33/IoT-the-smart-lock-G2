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

// Send door lock status (wrapper for MqttMessage::sendDeviceLock)
void mqtt_send_lock_status(const char* device_id, bool is_locked);

// Send alert message (wrapper for MqttMessage::sendDeviceLockAlert)
void mqtt_send_alert(const char* device_id, const char* alert_type);

// Get device ID from MAC address
void init_device_id(char* out_device_id, int max_len);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_WRAPPERS_H */