#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

class WifiManager
{
public:
    static void init(const char *ssid, const char *password);
    static void connect();

private:
    static void eventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
};

#endif // WIFI_MANAGER_H
