#ifndef _INCLUDE_WIFI_TIME_HPP_
#define _INCLUDE_WIFI_TIME_HPP_

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wps.h"
#include "esp_sntp.h"
#include "clock_common.hpp"
#include "mqtt_config.hpp"
#ifdef MQTT_ACTIVE
#include "mqtt_client.h"
#endif

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define WIFI_RECHECK_PERIOD_WITH_TIME_SYNC 900000   // 15 Minutes
#define WIFI_RECHECK_PERIOD_NO_TIME_SYNC   15000    // 15 seconds, we need a sync for the time!
#define WIFI_NR_RETRIES 3

class WifiTime {
    static void wifiEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    static void gotIPEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    static void monitorWifiTask(void *pvParameter);
    void monitorWifi(void);
    void initSTA(void);
    void initSNTP(void);
    #ifdef MQTT_ACTIVE
    void mqttAppStart(void);
    static void mqttEventHandler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data);
    #endif

    EventGroupHandle_t wifi_event_group;
    bool wifi_is_connected = false;
    uint8_t retry_num = 0;
    bool wps_is_active = false;
    wifi_credentials_t *wifi_credentials;
    #ifdef MQTT_ACTIVE
    esp_mqtt_client_handle_t mqtt_client = NULL;
    #endif

   public:
    void init(wifi_credentials_t *credentials);
    void startWPS(void);
    void stopWPS(void);
    bool isWPSActive(void);
    bool isTimeSet(void);
    bool isWifiConnected(void);
    void setTime(struct tm *timeinfo);
    void getTime(clock_time_t *time);
    #ifdef MQTT_ACTIVE
    bool isMQTTConnected(void);
    void sendMQTTAlarmTriggered(void);
    void sendMQTTAlarmStopped(void);
    #endif
};

#endif // _INCLUDE_WIFI_TIME_HPP_