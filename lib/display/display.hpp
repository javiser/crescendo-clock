#ifndef _INCLUDE_DISPLAY_HPP
#define _INCLUDE_DISPLAY_HPP

#include "freertos/queue.h"
#include "esp_adc/adc_oneshot.h"
#include "lgfx_ili9341.hpp"

#define DISPLAY_BRIGHTNESS_LEVELS_NR    4  // Not including the off-level!

#define DISPLAY_SYMBOL_WIFI_ON   ";"
#define DISPLAY_SYMBOL_WIFI_OFF  "<"
#define DISPLAY_SYMBOL_MQTT_ON   "="
#define DISPLAY_SYMBOL_MQTT_OFF  ">"
#define DISPLAY_SYMBOL_ALARM_ON  "?"
#define DISPLAY_SYMBOL_ALARM_OFF "@"
#define DISPLAY_SYMBOL_ALARM_L   "A"
#define DISPLAY_SYMBOL_ALARM_R   "B"
#define DISPLAY_SYMBOL_SNOOZE    "C"
#define DISPLAY_SYMBOL_BED       "D"
#define DISPLAY_SYMBOL_AUDIO_OFF "E"
#define DISPLAY_SYMBOL_WIFI_COG  "F"

typedef enum {
    D_E_TIME = 0,
    D_E_ALARM_TIME,
    D_E_ALARM_ACTIVE,
    D_E_BED_TIME,
    D_E_SNOOZE_TIME,
    D_E_SNOOZE_CANCEL,
    D_E_WIFI_STATUS,
    D_E_MQTT_STATUS,
    D_E_WIFI_SETTING,
    D_E_AUDIO,
} display_element_t;

typedef enum {
    D_A_OFF = 0,
    D_A_ON,
    D_A_HIDE_HOURS,
    D_A_HIDE_MINUTES,
    D_A_ONE_BAR,
    D_A_TWO_BARS,
} display_action_t;

typedef enum {
    D_LED_GREEN = 0,
    D_LED_RED,
} display_led_t;

class Display {
    LGFX_ILI9341 lcd;
    adc_oneshot_unit_handle_t adc1_handle;
    const uint16_t display_light_thd_down[DISPLAY_BRIGHTNESS_LEVELS_NR] = {24, 25, 60, 100};
    const uint16_t display_light_thd_up[DISPLAY_BRIGHTNESS_LEVELS_NR] = {30, 35, 80, 120};
    const uint8_t display_light_brightness[DISPLAY_BRIGHTNESS_LEVELS_NR + 1] = {0, 1, 10, 50, 100};
    uint8_t display_brightness_level = 2;
    bool max_brightness_requested = false;
    bool increased_brightness_requested = false;
    QueueHandle_t queue;
    bool show_alarm = false;

    static void monitorBrightnessTask(void *pvParameter);
    void setBrightness(uint8_t brightness_level);
    void controlBrightness(void);

   public:
    void init(void);
    void updateContent(display_element_t element, void *value, display_action_t action);
    void setMaxBrightness(bool request_max_brightness);
    void setIncreasedBrightness(bool request_inc_brightness);
    bool isDisplayOn(void);
};

#endif // _INCLUDE_DISPLAY_HPP