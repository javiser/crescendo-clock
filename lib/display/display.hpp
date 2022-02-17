#ifndef _INCLUDE_DISPLAY_HPP
#define _INCLUDE_DISPLAY_HPP

#include "freertos/queue.h"
#include "lgfx_ili9341.hpp"

#define DISPLAY_BRIGHTNESS_LEVELS_NR    4  // Not including the off-level!
#define LEDC_CHANNEL_RED                LEDC_CHANNEL_1  // Channel 0 reserved for display
#define LEDC_CHANNEL_GREEN              LEDC_CHANNEL_2  // Channel 0 reserved for display

typedef enum {
    D_E_TIME = 0,
    D_E_ALARM_TIME,
    D_E_SNOOZE_TIME,
    D_E_TEST,
} display_element_t;

typedef enum {
    D_A_OFF = 0,
    D_A_ON,
    D_A_HIDE_HOURS,
    D_A_HIDE_MINUTES,
    D_A_RIGHT,
    D_A_LEFT,
} display_action_t;

typedef enum {
    D_ANI_IDLE = 0,
    D_ANI_LEFT,
    D_ANI_RIGHT,
    D_ANI_STOP,
} display_animation_t;

typedef enum {
    D_LED_GREEN = 0,
    D_LED_RED,
} display_led_t;

class Display {
    LGFX_ILI9341 lcd;
    const uint16_t display_light_thd_down[DISPLAY_BRIGHTNESS_LEVELS_NR] = {40, 70, 100, 140};
    const uint16_t display_light_thd_up[DISPLAY_BRIGHTNESS_LEVELS_NR] = {50, 85, 120, 160};
    const uint8_t display_light_brightness[DISPLAY_BRIGHTNESS_LEVELS_NR + 1] = {0, 1, 10, 50, 100};
    uint8_t display_brightness_level = 0;
    bool max_brightness_requested = false;
    bool increased_brightness_requested = false;
    QueueHandle_t queue;
    bool show_alarm = false;
    // INFO LGFX_Sprite = 336 bytes without the sprite itself!
    LGFX_Sprite low_bg_sp;
    LGFX_Sprite alarm_time_sp;
    display_animation_t animate_action = D_ANI_IDLE;
    int16_t bg_sprite_x = 0;

    static void monitorBrightnessTask(void *pvParameter);
    static void animationTask(void *pvParameter);
    void drawLowerSection(void);
    void animateStuff(void);
    void setBrightness(uint8_t brightness_level);
    void controlBrightness(void);

   public:
    void init(void);
    void updateContent(display_element_t element, void *value, display_action_t action);
    void setMaxBrightness(bool request_max_brightness);
    void setIncreasedBrightness(bool request_inc_brightness);
    bool isDisplayOn(void);
    void setLEDsDuties(uint8_t red_duty, uint8_t green_duty);
    void fadeEffect(display_led_t led, uint16_t fade_time_on_ms, uint16_t fade_time_off_ms);
};

#endif // _INCLUDE_DISPLAY_HPP