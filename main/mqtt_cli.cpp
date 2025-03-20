#include "cloudComm/cloudCon.h"


// 确保 app_main 具有 C 链接性
extern "C" void app_main(void)
{
    // 初始化 Wi-Fi
    wifi_init();

    // 初始化 MQTT
    mqtt_init();

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}