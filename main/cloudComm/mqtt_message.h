#ifndef MQTT_MESSAGE_H
#define MQTT_MESSAGE_H

#include "mqtt_manager.h"
#include "json.hpp"
#include "esp_log.h"
#include <string>

using json = nlohmann::json;

class MqttMessage
{
public:
    /**
     * @brief 初始化MQTT系统，并注册处理后端发送消息的回调函数
     *
     * @param uri MQTT Broker 地址，如 "tcp://52.169.3.167:1883"
     * @param client_id 客户端ID
     * @param username 用户名 (如 "alice")
     * @param password 密码 (如 "alice123456")
     */
    static void init();

    /**
     * @brief 注册后端发送消息的回调函数
     *
     * 注册的主题有：
     * - "server/lock/code"：后端生成的密码
     * - "server/lock/all-code"：返回查询的所有密码
     */
    static void registerCallbacks();

    // 回调函数：处理 "server/lock/code" 主题的消息
    static void onServerLockCode(const json &data);

    // 回调函数：处理 "server/lock/all-code" 主题的消息
    static void onServerLockAllCode(const json &data);

    ////////// 发送消息接口 //////////

    /**
     * @brief 更新门锁状态
     *
     * 向主题 "device/lock" 发送如下消息：
     * {
     *   "deviceId": "10001",
     *   "isLocked": true
     * }
     *
     * @param device_id 设备ID
     * @param is_locked 门锁状态
     */
    static void sendDeviceLock(const std::string &device_id, bool is_locked);

    /**
     * @brief 确认收到密码
     *
     * 向主题 "device/lock/code" 发送如下消息：
     * {
     *   "deviceId": "10001",
     *   "codeId": "xxx",      // 替换成实际收到的 codeId
     *   "code": "xxx"         // 替换成实际收到的 code
     * }
     *
     * @param device_id 设备ID
     * @param codeId    密码ID
     * @param code      密码内容
     */
    static void sendDeviceLockCode(const std::string &device_id, const std::string &codeId, const std::string &code);

    /**
     * @brief 发送警报信息
     *
     * 向主题 "device/lock/alert" 发送如下消息：
     * {
     *   "deviceId": "10001",
     *   "type": "MOTOR"      // 例如 MOTOR, FINGERPRINT, SCREEN, LIGHT
     * }
     *
     * @param device_id 设备ID
     * @param alert_type 警报类型
     */
    static void sendDeviceLockAlert(const std::string &device_id, const std::string &alert_type);

    /**
     * @brief 请求获取所有密码
     *
     * 向主题 "device/lock/all-code" 发送如下消息：
     * {
     *   "deviceId": "10001"
     * }
     *
     * @param device_id 设备ID
     */
    static void requestDeviceAllCode(const std::string &device_id);
};

#endif // MQTT_MESSAGE_H
