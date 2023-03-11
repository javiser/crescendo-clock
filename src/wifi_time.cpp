#include "esp_log.h"
#include <wifi_time.hpp>
#include "credentials.hpp"

static const char *TAG = "wifi_time";

// I would have liked to add this as a class member but I don't know yet how to solve this
static bool mqtt_is_connected = false;

void WifiTime::wifiEventHandler(void *pvParameter, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    WifiTime *pThis = (WifiTime *)pvParameter;
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                pThis->wifi_is_connected = false;
                if (pThis->retry_num < WIFI_NR_RETRIES) {
                    esp_wifi_connect();
                    pThis->retry_num++;
                    ESP_LOGI(TAG, "retry number %d for connection to WiFi", pThis->retry_num);
                } else {
                    pThis->retry_num = 0;
                    xEventGroupSetBits(pThis->wifi_event_group, WIFI_FAIL_BIT);
                }
                break;
            case WIFI_EVENT_STA_WPS_ER_SUCCESS:
                // Copy obtained credentials to have them saved in NVS later
                wifi_config_t wifi_config;
                esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
                strcpy((char*)pThis->wifi_credentials->ssid, (char*)wifi_config.sta.ssid);
                strcpy((char*)pThis->wifi_credentials->password, (char*)wifi_config.sta.password);
                pThis->stopWPS();
                break;
            default:
                pThis->stopWPS();
                break;
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
        ESP_LOGI(TAG, "Connected to WiFi: %s", wifi_credentials->ssid);
        wifi_is_connected = true;
        mqttAppStart();
    } 
    else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to WiFi: %s", wifi_credentials->ssid);
    } else {
        ESP_LOGE(TAG, "Unexpected event");
    }
    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    // Depending on the timesync status
    if (isTimeSet())
        // Easy, we had a timesync, just wait 15 min before we try to reconnect again
        vTaskDelay(WIFI_RECHECK_PERIOD_WITH_TIME_SYNC / portTICK_PERIOD_MS);
    else
        // No timesync now, we want to keep trying. Wait 15 seconds only
        vTaskDelay(WIFI_RECHECK_PERIOD_NO_TIME_SYNC / portTICK_PERIOD_MS);

    if (!wps_is_active) // Otherwise the WPS process will be interrupted
        esp_wifi_connect();
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
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, this->wifiEventHandler, this, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, this->wifiEventHandler, this, &instance_got_ip));

    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, (char*)wifi_credentials->ssid);
    strcpy((char*)wifi_config.sta.password, (char*)wifi_credentials->password);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    esp_wifi_set_storage(WIFI_STORAGE_FLASH);

    xTaskCreate(this->monitorWifiTask, "monitor_wifi_task", 4096, this, 10, NULL);
}

void WifiTime::startWPS(void) {
    esp_wifi_disconnect();
    static esp_wps_config_t wps_config = WPS_CONFIG_INIT_DEFAULT(WPS_TYPE_PBC);
    ESP_ERROR_CHECK(esp_wifi_wps_enable(&wps_config));
    ESP_ERROR_CHECK(esp_wifi_wps_start(0));
    wps_is_active = true;
}

void WifiTime::stopWPS(void) {
    wps_is_active = false;
    ESP_ERROR_CHECK(esp_wifi_wps_disable());
    esp_wifi_connect();
}

bool WifiTime::isWPSActive(void) {
    return wps_is_active;
}

void WifiTime::initSNTP(void) {
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
    sntp_init();
    // Timezone Berlin: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
}

void WifiTime::init(wifi_credentials_t *credentials) {
    wifi_credentials = credentials;
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

void WifiTime::mqttAppStart(void) {
    esp_mqtt_client_config_t mqttConfig = {};
    mqttConfig.broker.address.uri = MQTT_BROKER_ADDRESS;
    mqttConfig.credentials.username = MQTT_USERNAME;
    mqttConfig.credentials.authentication.password = MQTT_PASSWORD;

    mqtt_client = esp_mqtt_client_init(&mqttConfig);
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ERROR, mqttEventHandler, NULL));
	ESP_ERROR_CHECK(esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_CONNECTED, mqttEventHandler, NULL));
	ESP_ERROR_CHECK(esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_DISCONNECTED, mqttEventHandler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(mqtt_client));
}

void WifiTime::mqttEventHandler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT is connected");				
            mqtt_is_connected = true;
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGE(TAG, "MQTT is disconnected");		
            mqtt_is_connected = false;
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR has been reported");
            break;
        default:
            ESP_LOGI(TAG, "MQTT event id:%d reported and not processed", event->event_id);
            break;
    }
}

bool WifiTime::isMQTTConnected(void)
{
    return mqtt_is_connected;
}

void WifiTime::sendMQTTAlarmTriggered(void)
{
    if (esp_mqtt_client_publish(mqtt_client, "wecker/wake_up", NULL, 0, 0, 0) == -1)
        ESP_LOGE(TAG, "Error sending MQTT message for alarm triggered");
}

void WifiTime::sendMQTTAlarmStopped(void)
{
    if (esp_mqtt_client_publish(mqtt_client, "wecker/alarm_off", NULL, 0, 0, 0) == -1)
        ESP_LOGE(TAG, "Error sending MQTT message for alarm stopped");
}