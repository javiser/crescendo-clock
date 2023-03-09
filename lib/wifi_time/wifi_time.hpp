#ifndef _INCLUDE_WIFI_TIME_HPP_
#define _INCLUDE_WIFI_TIME_HPP_

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_sntp.h"
#include "mqtt_client.h"
// TODO it should be possible to add this without relative paths
#include "../../src/clock_common.hpp"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// TODO For development purposes I want a shorter time (why??), 900 000 is a better value
#define WIFI_RECHECK_PERIOD_WITH_TIME_SYNC 10000
#define WIFI_RECHECK_PERIOD_NO_TIME_SYNC 15000
#define WIFI_NR_RETRIES 3

class WifiTime {
    static void wifiEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    static void monitorWifiTask(void *pvParameter);
    void monitorWifi(void);
    void initSTA(void);
    void initSNTP(void);
    void mqttAppStart(void);
    static void mqttEventHandler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data);

    EventGroupHandle_t wifi_event_group;
    bool wifi_is_connected = false;
    uint8_t retry_num = 0;
    esp_mqtt_client_handle_t mqtt_client = NULL;

   public:
    void init(void);
    bool isTimeSet(void);
    bool isWifiConnected(void);
    void setTime(struct tm *timeinfo);
    void getTime(clock_time_t *time);
    bool isMQTTConnected(void);
    void wakeUpLight(void);
    void stopWakeUpLight(void);
};

#endif // _INCLUDE_WIFI_TIME_HPP_