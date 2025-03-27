#include "mqtt_manager.h"
#include "mqtt_message.h"

static const char *TAG = "MqttManager";

esp_mqtt_client_handle_t MqttManager::client = nullptr;
std::unordered_map<std::string, std::function<void(const json &)>> MqttManager::callbacks;

extern const uint8_t isrgrootx1_pem_start[] asm("_binary_isrgrootx1_pem_start");
extern const uint8_t isrgrootx1_pem_end[] asm("_binary_isrgrootx1_pem_end");

void MqttManager::init(const char *uri, const char *client_id, const char *username, const char *password)
{
    ESP_LOGI(TAG, "Initializing MQTT with broker: %s", uri);

    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = uri;
    mqtt_cfg.credentials.client_id = client_id;
    mqtt_cfg.credentials.username = username;
    mqtt_cfg.credentials.authentication.password = password;
    mqtt_cfg.session.keepalive = 60; // 保持连接心跳时间（秒）

    mqtt_cfg.broker.verification.certificate = (const char *)isrgrootx1_pem_start;
    mqtt_cfg.broker.verification.certificate_len = isrgrootx1_pem_end - isrgrootx1_pem_start;
    ESP_LOGI(TAG, "Cert len: %d", mqtt_cfg.broker.verification.certificate_len);

    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == nullptr)
    {
        ESP_LOGE(TAG, "MQTT client initialization failed!");
        return;
    }
    else
    {
        ESP_LOGI(TAG, "MQTT client initialized successfully!");
    }

    // 注册所有事件，使用统一的 eventHandler 分发
    esp_mqtt_client_register_event(client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, MqttManager::eventHandler, client);
    esp_mqtt_client_start(client);
}

void MqttManager::registerCallback(const std::string &topic, std::function<void(const json &)> callback)
{
    callbacks[topic] = callback;
    if (client)
    {
        // 订阅对应的 topic（这里采用每注册一次就订阅该 topic 的方式）
        esp_mqtt_client_subscribe(client, topic.c_str(), 0);
        ESP_LOGI(TAG, "Subscribed to topic: %s", topic.c_str());
    }
    else
    {
        ESP_LOGE(TAG, "MQTT client is not initialized. Cannot subscribe to topic: %s", topic.c_str());
    }
}

void MqttManager::eventHandler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    if (!client)
    {
        ESP_LOGE(TAG, "MQTT client is not initialized!");
        return;
    }

    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        // 此处可以选择在连接后订阅默认主题或留给 registerCallback 动态订阅
        MqttMessage::registerCallbacks();
        break;

    case MQTT_EVENT_DATA:
    {
        // 将 topic 和数据转换为 std::string
        std::string topic(event->topic, event->topic_len);
        std::string data(event->data, event->data_len);
        ESP_LOGI(TAG, "MQTT_EVENT_DATA, topic=%s, data=%s", topic.c_str(), data.c_str());

        if (data.empty())
        {
            ESP_LOGE(TAG, "MQTT received empty payload!");
            break;
        }

        // 解析 JSON 数据
        auto json_obj = parseJson(data);
        if (json_obj.has_value())
        {
            // 查找是否有针对该 topic 的注册回调函数
            auto it = callbacks.find(topic);
            if (it != callbacks.end())
            {
                // 调用注册的回调函数，并传入解析后的 JSON 对象
                it->second(*json_obj);
            }
            else
            {
                ESP_LOGW(TAG, "No callback registered for topic: %s", topic.c_str());
            }
        }
        else
        {
            ESP_LOGE(TAG, "JSON Parsing Failed!");
        }
        break;
    }
    case MQTT_EVENT_ERROR:
    {
        ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
        break;
    }
    case MQTT_EVENT_BEFORE_CONNECT:
    {
        ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
        break;
    }
    case MQTT_EVENT_DISCONNECTED:
    {
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    }
    case MQTT_EVENT_SUBSCRIBED:
    {
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    }
    default:
        ESP_LOGI(TAG, "Unhandled MQTT event: %d", event->event_id);
        break;
    }
}

void MqttManager::sendJson(const std::string &topic, const json &json_data)
{
    if (!client)
    {
        ESP_LOGE(TAG, "MQTT client is not initialized!");
        return;
    }

    std::string json_str = json_data.dump();
    esp_mqtt_client_publish(client, topic.c_str(), json_str.c_str(), 0, 0, 0);
    ESP_LOGI(TAG, "Sent JSON message to topic %s: %s", topic.c_str(), json_str.c_str());
}

std::optional<json> MqttManager::parseJson(const std::string &json_str)
{
    json parsed_json = json::parse(json_str, nullptr, false); // 关闭异常模式

    if (parsed_json.is_discarded())
    {
        ESP_LOGE(TAG, "JSON Parsing Failed: Invalid JSON!");
        return std::nullopt;
    }

    return parsed_json;
}
