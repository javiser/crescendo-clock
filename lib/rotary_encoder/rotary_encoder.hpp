#ifndef _INCLUDE_ROTARY_ENCODER_HPP
#define _INCLUDE_ROTARY_ENCODER_HPP

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#define DEBOUNCE_MASK   0b1111000000111111
#define LONG_PRESS_DURATION (1000)

typedef int16_t rotary_encoder_pos_t;

typedef enum {
    DIR_NONE = 0,
    DIR_RIGHT,
    DIR_LEFT,
} rotary_encoder_dir_t;

typedef enum {
    ENCODER_ROTATION = 0,
    BUTTON_SHORT_PRESS,
    BUTTON_LONG_PRESS,
} rotary_encoder_event_type_t;


typedef struct {
    bool inverted;
    uint16_t history;
    uint32_t down_time;
} button_debounce_t;

typedef struct
{
    rotary_encoder_event_type_t type;
    rotary_encoder_pos_t position;
    rotary_encoder_dir_t direction;
} rotary_encoder_event_t;

class RotaryEncoder {
    gpio_num_t pin_a;
    gpio_num_t pin_b;
    gpio_num_t pin_button;
    button_debounce_t debounce;
    QueueHandle_t queue;
    uint8_t encoder_state = 0;
    rotary_encoder_dir_t encoder_started_rotation = DIR_NONE;
    rotary_encoder_pos_t position;
    rotary_encoder_dir_t direction;
    rotary_encoder_pos_t min_position;
    rotary_encoder_pos_t max_position;
    rotary_encoder_pos_t step_increment;
    bool wrap_values;

    void updateButton(button_debounce_t *d);
    bool buttonRose(button_debounce_t *d);
    bool buttonFell(button_debounce_t *d);
    bool buttonDown(button_debounce_t *d);
    bool buttonUp(button_debounce_t *d);
    uint32_t getTimeInMs();
    void sendButtonEvent(rotary_encoder_event_type_t event_type);
    static void buttonTask(void *pvParameter);
    void processButton();
    static void rotationInterruptHandler(void *pvParameter);
    void processEncoderInterrupt();

   public:
    QueueHandle_t init(gpio_num_t pin_a, gpio_num_t pin_b, gpio_num_t pin_button, bool inverted);
    void setRange(rotary_encoder_pos_t min, rotary_encoder_pos_t max, rotary_encoder_pos_t step, bool wrap);
    void setPosition(rotary_encoder_pos_t position);
    rotary_encoder_pos_t getPosition();
};

#endif  // _INCLUDE_ROTARY_ENCODER_HPP
