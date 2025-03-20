
# List of components:

fingerprint sensor: [AS608](https://gleanntronics.ie/en/products/as608-optical-fingerprint-sensor-jm-101b-fingerprint-matching-4040.html?query_id=2)

RFID Module: [RFID-RC522](https://gleanntronics.ie/en/products/rc522-13-56mhz-rfid-reader-module-card-key-fob-arduino-75.html?gad_source=1&gclid=CjwKCAiA2JG9BhAuEiwAH_zf3gcg8qu53gT0VbhYUyS9kyr-O5qhl11eODomB_6cjYMp9f-47O4DaRoCIkoQAvD_BwE#close)

PIR Sensor: [HC-SR501](https://gleanntronics.ie/en/products/hc-sr501-motion-detector-module-pir-sensor-arduino-228.html?query_id=3)

OLED Screen: [SSD1306](https://gleanntronics.ie/en/products/ssd1306-oled-display-0-96-blue-i2c-3-5v-arduino-265.html?query_id=4)

Keyboard: [TTP229](https://gleanntronics.ie/en/products/16-button-touch-keyboard-ttp229-touch-sensor-for-arduino-5101.html?query_id=8)

# 客户端接口文档

## App -> 后端 发送数据

http://63.32.52.62/swagger-ui/index.html

### 注册

```plaintext
POST http://63.32.52.62/api/user/register
{
    "username": "alfa",
    "password": "123456",
    "email": "a@b.cd"
}
```

### 登录

```plaintext
GET http://63.32.52.62/api/user/login
Authorization: Basic alfa:123456（alfa:123456需要经过base64编码）
```

### 查询设备

```plaintext
GET http://63.32.52.62/api/device
Authorization: Bearer JWT 
```

### 请求生成密码

```plaintext
POST http://63.32.52.62/api/code
Authorization: Bearer JWT
{
    "deviceId": 10001,
    "validFrom": "2025-03-22T00:00+08:00",
    "validTo": "2027-03-05T00:30+00:00"
}
```

### 重置（找回）密码

```plaintext
PUT http://63.32.52.62/api/user/reset
{
  "username": "alfa",
  "password": "654321",
  "email": "a@b.cd"
}
```

## WebSocket

WebSocket端点 /websocket

订阅 /user/queue/alert 接收设备警报

订阅 /user/queue/is-locked 接收门锁状态

# 单片机端接口文档

tcp://52.169.3.167:1883

## 后端 -> ESP32(Broker) 发送数据

### 后端生成的密码

server/lock/code

```json
{
  "code": "645941",
  "codeId": 1893389214167859200,
  "deviceId": 10001,
  "validFrom": "2025-03-22T00:00+08:00",
  "validTo": "2026-02-22T00:00+08:00"
}
```

### 返回查询的所有密码

server/lock/all-code

```json
[
  {
    "code": "769859",
    "codeId": 1897785585675399168,
    "deviceId": 10001,
    "validFrom": "2025-03-21T16:00:00[GMT]",
    "validTo": "2027-03-05T00:30:00[GMT]"
  },
  {
    "code": "414697",
    "codeId": 1898098220849545216,
    "deviceId": 10001,
    "validFrom": "2025-03-21T16:00:00[GMT]",
    "validTo": "2027-03-05T00:30:00[GMT]"
  }
]
```

## ESP32 -> 后端(Broker) 发送数据

username: alice

password: alice123456

### 更新门锁状态

device/lock

```json
{
  "deviceId": "10001",
  "isLocked": true
}
```

### 确认收到密码

device/lock/code

```json
{
  "deviceId": "10001",
  "codeId": "替换成收到的codeId",
  "code": "替换成收到的code"
}
```

### 发送警报

device/lock/alert

```json
{
  "deviceId": "10001",
  "type": "MOTOR"
}
```

> 电机坏了写：MOTOR
>
> 指纹坏了写：FINGERPRINT
>
> 屏幕坏了写：SCREEN
>
> 灯泡坏了写：LIGHT

### 获取密码

device/lock/all-code

```json
{
  "deviceId": "10001"
}
```
