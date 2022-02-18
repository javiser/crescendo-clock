#include <string.h>
#include <wifi_time.hpp>

// TODO Actually I am not sure what is the downside of using ESP_LOG. Maybe more stack demand? Runtime? No idea
//#define _DEBUG
#ifdef _DEBUG
#include "esp_log.h"
static const char *TAG = "wifi_time";
#endif

void WifiTime::eventHandler(void *pvParameter, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    WifiTime *pThis = (WifiTime *)pvParameter;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        pThis->wifi_is_connected = false;
        if (pThis->retry_num < WIFI_NR_RETRIES) {
            esp_wifi_connect();
            pThis->retry_num++;
            #ifdef _DEBUG
            ESP_LOGI(TAG, "retry number %d for connection to AP", pThis->retry_num);
            #endif
        } else {
            pThis->retry_num = 0;
            xEventGroupSetBits(pThis->wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        pThis->retry_num = 0;
        xEventGroupSetBits(pThis->wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void WifiTime::monitorWifiTask(void *pvParameter) {
    WifiTime *pThis = (WifiTime *)pvParameter;
    ESP_ERROR_CHECK(esp_wifi_start());

    while (1) {
        pThis->monitorWifi();
    }
}

void WifiTime::monitorWifi(void) {
    // Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
    // number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above)
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    // xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually happened.
    if (bits & WIFI_CONNECTED_BIT) {
        #ifdef _DEBUG
        ESP_LOGI(TAG, "Connected to: %s", ESP_WIFI_SSID);
        #endif
        wifi_is_connected = true;
    } 
    #ifdef _DEBUG
    else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to: %s", ESP_WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    #endif
    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    // Depending on the timesync status
    if (isTimeSet())
        // Easy, we had a timesync, just wait 15 min before we try to reconnect again
        vTaskDelay(WIFI_RECHECK_PERIOD_WITH_TIME_SYNC / portTICK_PERIOD_MS);
    else
        // No timesync now, we want to keep trying. Wait 15 seconds only
        vTaskDelay(WIFI_RECHECK_PERIOD_NO_TIME_SYNC / portTICK_PERIOD_MS);

    #ifdef _DEBUG
    if (!wifi_is_connected)
        ESP_LOGI(TAG, "Not connected yet, let's try again to reconnect");
    #endif
    ESP_ERROR_CHECK(esp_wifi_connect());
}

void WifiTime::initSTA(void) {
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, this->eventHandler, this, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, this->eventHandler, this, &instance_got_ip));

    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, ESP_WIFI_SSID);
    strcpy((char*)wifi_config.sta.password, ESP_WIFI_PASS);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    esp_wifi_set_storage(WIFI_STORAGE_FLASH);

    // TODO I decreased the stack size from 4096 to 1024 and seems to be working fine
    xTaskCreate(this->monitorWifiTask, "monitor_wifi_task", 1024, this, 10, NULL);
}

void WifiTime::initSNTP(void) {
    #ifdef _DEBUG
    ESP_LOGI(TAG, "Initializing SNTP");
    #endif
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
    sntp_init();
    // Timezone Berlin: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
}

void WifiTime::init(void) {
    sntp_servermode_dhcp(0);
    initSTA();
    initSNTP();
}

bool WifiTime::isTimeSet(void) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    return (timeinfo.tm_year > (1970 - 1900));
}

bool WifiTime::isWifiConnected(void) {
    return wifi_is_connected;
}

void WifiTime::setTime(struct tm *timeinfo) {
    // TODO I am not interested in the date I believe
    // timeinfo->tm_year = 2022 - 1900;
    // timeinfo->tm_mon = 0;
    // timeinfo->tm_mday = 26;
    // timeinfo->tm_hour = 13;
    // timeinfo->tm_min = 58;
    // timeinfo->tm_sec = 30;
    time_t t = mktime(timeinfo);
    struct timeval now_set = {.tv_sec = t, .tv_usec = 0};
    settimeofday(&now_set, NULL);
}

void WifiTime::getTime(clock_time_t *t) {
    // Get current RTC time from the ESP32
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    t->hour = (uint8_t)timeinfo.tm_hour;
    t->minute = (uint8_t)timeinfo.tm_min;
}