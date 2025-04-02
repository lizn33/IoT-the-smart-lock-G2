
# List of components:

fingerprint sensor: [AS608](https://gleanntronics.ie/en/products/as608-optical-fingerprint-sensor-jm-101b-fingerprint-matching-4040.html?query_id=2)

RFID Module: [RFID-RC522](https://gleanntronics.ie/en/products/rc522-13-56mhz-rfid-reader-module-card-key-fob-arduino-75.html?gad_source=1&gclid=CjwKCAiA2JG9BhAuEiwAH_zf3gcg8qu53gT0VbhYUyS9kyr-O5qhl11eODomB_6cjYMp9f-47O4DaRoCIkoQAvD_BwE#close)

PIR Sensor: [HC-SR501](https://gleanntronics.ie/en/products/hc-sr501-motion-detector-module-pir-sensor-arduino-228.html?query_id=3)

OLED Screen: [SSD1306](https://gleanntronics.ie/en/products/ssd1306-oled-display-0-96-blue-i2c-3-5v-arduino-265.html?query_id=4)

Keyboard: [TTP229](https://gleanntronics.ie/en/products/16-button-touch-keyboard-ttp229-touch-sensor-for-arduino-5101.html?query_id=8)

# 客户端接口文档

## App -> 后端 发送数据

https://8117.me/swagger-ui/index.html

### 注册

```plaintext
POST https://8117.me/api/user/register
{
    "username": "alfa",
    "password": "123456",
    "email": "a@b.cd"
}
```

### 登录

```plaintext
GET https://8117.me/api/user/login
Authorization: Basic alfa:123456（alfa:123456需要经过base64编码）
```

### 重置（找回）密码

```plaintext
PUT https://8117.me/api/user/reset
{
  "username": "alfa",
  "password": "654321",
  "email": "a@b.cd"
}
```

### 获取用户信息

```plaintext
GET https://8117.me/api/user
Authorization: Bearer JWT
```

### 获取设备信息

```plaintext
GET https://8117.me/api/device
Authorization: Bearer JWT 
```

### 获取用户生成的密码

```plaintext
GET https://8117.me/api/code
Authorization: Bearer JWT 
```

### 请求生成密码

```plaintext
POST https://8117.me/api/code
Authorization: Bearer JWT
{
    "deviceId": 10001,
    "validFrom": "2025-03-22T00:00+08:00",
    "validTo": "2027-03-05T00:30+00:00"
}
```

### 上传文件

```plaintext
POST https://8117.me/api/file
Authorization: Bearer JWT
Body: form-data{
    Key=file
    Type=File
    Value=上传的文件
}
```

### 下载文件

```plaintext
GET https://8117.me/api/file/{fileName}
Authorization: Bearer JWT
```

## WebSocket

WebSocket端点 /websocket

订阅 /user/queue/alert 接收设备警报

订阅 /user/queue/is-locked 接收门锁状态

# 单片机端接口文档

mqtts://emqx.8117.me:8883

## 后端 -> ESP32(Broker) 发送数据

### OTA更新通知

server/lock/ota

```json
{
  "deviceId": "10001",
  "version": "v1.01"
}
```

### 后端生成的密码

server/lock/code

```json
{
  "code": "645941",
  "codeId": "1893389214167859200",
  "deviceId": "10001",
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
    "codeId": "1897785585675399168",
    "deviceId": "10001",
    "validFrom": "2025-03-21T16:00:00[GMT]",
    "validTo": "2027-03-05T00:30:00[GMT]"
  },
  {
    "code": "414697",
    "codeId": "1898098220849545216",
    "deviceId": "10001",
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


# Smart Door Lock System

This project implements a smart door lock system using an ESP32 microcontroller. The system integrates multiple components such as an OLED display, RFID reader, keypad, servo motor, and LED to provide a secure and user-friendly locking mechanism.

## Features

- **RFID Authentication**: Supports RFID cards for unlocking the door.
- **Keypad Authentication**: Allows users to enter a password to unlock the door.
- **OLED Display**: Displays system status, password input, and messages.
- **Servo Motor Control**: Locks and unlocks the door.
- **LED Indicator**: Provides visual feedback for system status.
- **Lockout Mechanism**: Locks the system after multiple failed attempts.
- **Auto-lock**: Automatically locks the door after a set duration.

## Components

### 1. **RFID Reader**
- Uses the RC522 RFID module to read card UIDs.
- Supports adding and verifying authorized UIDs.
- Handles card detection and communication via SPI.

### 2. **Keypad**
- 4x4 matrix keypad for password input.
- Supports password entry, clearing, and confirmation.
- Implements a lockout mechanism after multiple failed attempts.

### 3. **OLED Display**
- Displays system status, password input, and messages.
- Provides visual feedback for user interactions.
- Uses I2C communication for data transfer.

### 4. **Servo Motor**
- Controls the locking and unlocking mechanism.
- Supports calibration and precise control of rotation.
- Uses PWM signals for operation.

### 5. **LED Indicator**
- Provides visual feedback for system status.
- Flashes during system startup, password entry, and errors.

## Detailed Pin Instructions

Below is a suggested wiring guide for connecting the components to the ESP32:

| Component    | Signal    | ESP32 Pin   | Notes                                         |
|--------------|-----------|-------------|-----------------------------------------------|
| **RFID (RC522)** | MOSI      | GPIO 23    | SPI Master Out Slave In                       |
|              | MISO      | GPIO 19    | SPI Master In Slave Out                       |
|              | SCK       | GPIO 18    | SPI Clock                                     |
|              | CS        | GPIO 5     | SPI Chip Select                               |
|              | RST       | GPIO 25    | Reset pin (assigned to avoid I2C conflict)    |
| **Keypad**   | Row 1     | GPIO 32    | Configurable in `config.h`                    |
|              | Row 2     | GPIO 33    |                                               |
|              | Row 3     | GPIO 34    |                                               |
|              | Row 4     | GPIO 35    |                                               |
|              | Col 1     | GPIO 26    |                                               |
|              | Col 2     | GPIO 27    |                                               |
|              | Col 3     | GPIO 14    |                                               |
|              | Col 4     | GPIO 12    |                                               |
| **OLED Display** | SDA      | GPIO 21    | I2C Data                                      |
|              | SCL      | GPIO 22    | I2C Clock                                     |
| **Servo Motor** | PWM     | GPIO 13    | PWM output for controlling the servo          |
| **LED Indicator** | LED     | GPIO 2     | Output for status indication                  |