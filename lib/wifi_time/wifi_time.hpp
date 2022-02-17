#ifndef _INCLUDE_WIFI_TIME_HPP_
#define _INCLUDE_WIFI_TIME_HPP_

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_sntp.h"
// TODO it should be possible to add this without relative paths
#include "../../src/clock_common.hpp"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
// TODO Replace or just use WPS
//#define ESP_WIFI_SSID      "WLAN fuer unsere liebe Gaeste"
//#define ESP_WIFI_PASS      "mojakochanajestbardzoladna"
#define ESP_WIFI_SSID      "iPhone privado"
#define ESP_WIFI_PASS      "nadxguw06jw1x"

// TODO For development purposes I want a shorter time (why??), 900 000 is a better value
#define WIFI_RECHECK_PERIOD_WITH_TIME_SYNC 10000
#define WIFI_RECHECK_PERIOD_NO_TIME_SYNC 15000
#define WIFI_NR_RETRIES 3

class WifiTime {
    static void eventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    static void monitorWifiTask(void *pvParameter);
    void monitorWifi(void);
    void initSTA(void);
    void initSNTP(void);

    EventGroupHandle_t wifi_event_group;
    bool wifi_is_connected = false;
    uint8_t retry_num = 0;

   public:
    void init(void);
    bool isTimeSet(void);
    bool isWifiConnected(void);
    void setTime(struct tm *timeinfo);
    void getTime(clock_time_t *time);
};

#endif // _INCLUDE_WIFI_TIME_HPP_