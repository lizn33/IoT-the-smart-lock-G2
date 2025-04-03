#include "mqtt_message.h"
#include "password.h"

static const char *TAG = "MqttMessage";

void MqttMessage::init()
{
    ESP_LOGI(TAG, "Initializing MQTT Message system");
    // 初始化底层MQTT客户端
    // MqttManager::init(uri, client_id, username, password);

    // 注册后端发送消息的回调函数
    // 放入MQTT_EVENT_CONNECTED事件处理中，避免在未连接时注册回调
    // registerCallbacks();
}

void MqttMessage::registerCallbacks()
{
    // 注册 "server/lock/ota" 的回调函数
    MqttManager::registerCallback("server/lock/ota", onServerLockOta);
    ESP_LOGI(TAG, "Registered callback for topic: server/lock/ota");

    // 注册 "server/lock/code" 的回调函数
    MqttManager::registerCallback("server/lock/code", onServerLockCode);
    ESP_LOGI(TAG, "Registered callback for topic: server/lock/code");

    // 注册 "server/lock/all-code" 的回调函数
    MqttManager::registerCallback("server/lock/all-code", onServerLockAllCode);
    ESP_LOGI(TAG, "Registered callback for topic: server/lock/all-code");
}

void MqttMessage::onServerLockOta(const json &data)
{
    const std::string url_prefix = "https://8117.me/api/file/";
    // 从 JSON 数据中获取版本号作为文件名（确保 data["version"] 存在且格式正确）
    std::string file_name = data["version"];
    std::string url = url_prefix + file_name;

    ESP_LOGI(TAG, "Received OTA update notification, update URL: %s", url.c_str());

    // // 先创建 HTTP 配置实例
    // esp_http_client_config_t http_config = {};
    // http_config.url = url.c_str();
    // http_config.cert_pem = (const char *)isrgrootx1_pem_start;
    // http_config.timeout_ms = 10000; // 可根据需要调整超时时间

    // // 然后创建 HTTPS OTA 配置，并将 http_config 的地址赋给 http_config 成员
    // esp_https_ota_config_t ota_config = {};
    // ota_config.http_config = &http_config;

    // ESP_LOGI(TAG, "Starting OTA update...");
    // esp_err_t ret = esp_https_ota(&ota_config);
    // if (ret == ESP_OK)
    // {
    //     ESP_LOGI(TAG, "OTA update successful, restarting system...");
    //     esp_restart();
    // }
    // else
    // {
    //     ESP_LOGE(TAG, "OTA update failed, error = %d", ret);
    //     // 根据需要增加错误处理或重试逻辑
    // }
}

void MqttMessage::onServerLockCode(const json &data)
{
    // TODO: 在此处理后端生成的密码消息
    ESP_LOGI(TAG, "Received server/lock/code message");
    add_password(to_string(data["code"]).c_str(), to_string(data["validTo"]).c_str());

    MqttMessage::sendDeviceLockCode(to_string(data["deviceId"]), data["codeId"], data["code"]);

    // 例如：可以解析 data 中的 "code", "codeId", "deviceId", "validFrom", "validTo"
}

void MqttMessage::onServerLockAllCode(const json &data)
{
    // TODO: 在此处理返回查询所有密码的消息

    ESP_LOGI(TAG, "Received server/lock/all-code message");

    for (const auto &item : data)
    {
        add_password(to_string(item["code"]).c_str(), to_string(item["validTo"]).c_str());
    }
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
