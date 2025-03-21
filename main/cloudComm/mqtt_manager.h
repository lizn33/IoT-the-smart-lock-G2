#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "mqtt_client.h"
#include "esp_log.h"
#include "json.hpp"
#include <optional>
#include <string>
#include <functional>
#include <unordered_map>

using json = nlohmann::json;

class MqttManager
{
public:
    // 初始化 MQTT 客户端
    static void init(const char *uri, const char *client_id, const char *username, const char *password);

    // 发送 JSON 消息
    static void sendJson(const std::string &topic, const json &json_data);

    // 注册回调函数：收到指定 topic 的消息时会调用对应的回调函数
    static void registerCallback(const std::string &topic, std::function<void(const json &)> callback);

    // 解析 JSON 字符串
    static std::optional<json> parseJson(const std::string &json_str);

private:
    static void eventHandler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

    static esp_mqtt_client_handle_t client;
    // 存储 topic 与回调函数的映射
    static std::unordered_map<std::string, std::function<void(const json &)>> callbacks;
};

#endif // MQTT_MANAGER_H
