#include "mqtt_message.h"


static const char *TAG = "MqttMessage";

void MqttMessage::init()
{
    ESP_LOGI(TAG, "Initializing MQTT Message system");
    // 初始化底层MQTT客户端
    // MqttManager::init(uri, client_id, username, password);

    // 注册后端发送消息的回调函数
    registerCallbacks();
}

void MqttMessage::registerCallbacks()
{
    // 注册 "server/lock/code" 的回调函数
    MqttManager::registerCallback("server/lock/code", onServerLockCode);
    ESP_LOGI(TAG, "Registered callback for topic: server/lock/code");

    // 注册 "server/lock/all-code" 的回调函数
    MqttManager::registerCallback("server/lock/all-code", onServerLockAllCode);
    ESP_LOGI(TAG, "Registered callback for topic: server/lock/all-code");
}

void MqttMessage::onServerLockCode(const json &data)
{
    // TODO: 在此处理后端生成的密码消息
    ESP_LOGI(TAG, "Received server/lock/code message");
    // 例如：可以解析 data 中的 "code", "codeId", "deviceId", "validFrom", "validTo"
}

void MqttMessage::onServerLockAllCode(const json &data)
{
    // TODO: 在此处理返回查询所有密码的消息
    ESP_LOGI(TAG, "Received server/lock/all-code message");
    // 例如：解析 data 数组中的每个密码信息
}

///////////////////////////
// 发送消息接口实现
///////////////////////////

void MqttMessage::sendDeviceLock(const std::string &device_id, bool is_locked)
{
    json payload = {
        {"deviceId", device_id},
        {"isLocked", is_locked}};
    MqttManager::sendJson("device/lock", payload);
    ESP_LOGI(TAG, "Sent device/lock message: %s", payload.dump().c_str());
}

void MqttMessage::sendDeviceLockCode(const std::string &device_id, const std::string &codeId, const std::string &code)
{
    json payload = {
        {"deviceId", device_id},
        {"codeId", codeId},
        {"code", code}};
    MqttManager::sendJson("device/lock/code", payload);
    ESP_LOGI(TAG, "Sent device/lock/code message: %s", payload.dump().c_str());
}

void MqttMessage::sendDeviceLockAlert(const std::string &device_id, const std::string &alert_type)
{
    json payload = {
        {"deviceId", device_id},
        {"type", alert_type}};
    MqttManager::sendJson("device/lock/alert", payload);
    ESP_LOGI(TAG, "Sent device/lock/alert message: %s", payload.dump().c_str());
}

void MqttMessage::requestDeviceAllCode(const std::string &device_id)
{
    json payload = {
        {"deviceId", device_id}};
    MqttManager::sendJson("device/lock/all-code", payload);
    ESP_LOGI(TAG, "Sent device/lock/all-code request: %s", payload.dump().c_str());
}
