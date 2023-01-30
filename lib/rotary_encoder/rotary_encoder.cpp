#include "rotary_encoder.hpp"
#include "esp_timer.h"

void RotaryEncoder::updateButton(button_debounce_t *d) {
    d->history = (d->history << 1) | gpio_get_level(pin_button);
}

bool RotaryEncoder::buttonRose(button_debounce_t *d) {
    if ((d->history & DEBOUNCE_MASK) == 0b0000000000111111) {
        d->history = 0xffff;
        return 1;
    }
    return 0;
}

bool RotaryEncoder::buttonFell(button_debounce_t *d) {
    if ((d->history & DEBOUNCE_MASK) == 0b1111000000000000) {
        d->history = 0x0000;
        return 1;
    }
    return 0;
}

bool RotaryEncoder::buttonDown(button_debounce_t *d) {
    if (d->inverted) return buttonFell(d);
    return buttonRose(d);
}

bool RotaryEncoder::buttonUp(button_debounce_t *d) {
    if (d->inverted) return buttonRose(d);
    return buttonFell(d);
}

uint32_t RotaryEncoder::getTimeInMs() {
    return esp_timer_get_time() / 1000;
}

void RotaryEncoder::sendButtonEvent(rotary_encoder_event_type_t event_type) {
    rotary_encoder_event_t event =
        {
            .type = event_type,
            .position = position,
            .direction = direction,
        };
    xQueueSend(queue, &event, portMAX_DELAY);
}

void RotaryEncoder::buttonTask(void *pvParameter) {
    RotaryEncoder *pThis = (RotaryEncoder *)pvParameter;
    while (1) {
        pThis->processButton();
    }
}

void RotaryEncoder::processButton() {
    updateButton(&debounce);
    if (buttonUp(&debounce) && debounce.down_time) {
        debounce.down_time = 0;
        sendButtonEvent(BUTTON_SHORT_PRESS);
    } else if (debounce.down_time &&
               (getTimeInMs() >= (debounce.down_time + LONG_PRESS_DURATION))) {
        debounce.down_time = 0;  // To avoid sending BUTTON_SHORT_PRESS event
        sendButtonEvent(BUTTON_LONG_PRESS);
    } else if (buttonDown(&debounce) && (debounce.down_time == 0)) {
        debounce.down_time = getTimeInMs();
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

void RotaryEncoder::rotationInterruptHandler(void *pvParameter) {
    RotaryEncoder *pThis = (RotaryEncoder *)pvParameter;
    pThis->processEncoderInterrupt();
}

void RotaryEncoder::processEncoderInterrupt() {
    int s = encoder_state & 3;
    if (gpio_get_level(pin_a)) s |= 8;
    if (gpio_get_level(pin_b)) s |= 4;

    switch (s) {
        case 1:
        case 7:
        case 8:
            encoder_started_rotation = DIR_RIGHT;
            break;
        case 14:
            if (encoder_started_rotation == DIR_RIGHT) {
                position += step_increment;
                if (position > max_position) {
                    position = wrap_values ? min_position : max_position;
                }
                direction = DIR_RIGHT;
            }
            encoder_started_rotation = DIR_NONE;
            break;

        case 2:
        case 4:
        case 11:
            encoder_started_rotation = DIR_LEFT;
            break;
        case 13:
            if (encoder_started_rotation == DIR_LEFT) {
                position -= step_increment;
                if (position < min_position) {
                    position = wrap_values ? max_position : min_position;
                }
                direction = DIR_LEFT;
            }
            encoder_started_rotation = DIR_NONE;
            break;
        default:
            break;
    }
    encoder_state = (s >> 2);

    if (direction != DIR_NONE) {
        rotary_encoder_event_t event =
            {
                .type = ENCODER_ROTATION,
                .position = position,
                .direction = direction,
            };
        BaseType_t higher_priority_task_woken = pdFALSE;
        xQueueOverwriteFromISR(queue, &event, &higher_priority_task_woken);
        if (higher_priority_task_woken == pdTRUE) {
            portYIELD_FROM_ISR();
        }
    }

    direction = DIR_NONE;  // Reset value after sending event to get ready for the next one
}

QueueHandle_t RotaryEncoder::init(gpio_num_t A, gpio_num_t B, gpio_num_t button, bool inverted) {
    pin_a = A;
    pin_b = B;
    pin_button = button;
    position = 0;
    direction = DIR_NONE;
    min_position = 0;  // Some "random" initial values to get started
    max_position = 100;
    step_increment = 1;
    wrap_values = false;

    gpio_install_isr_service(0);
    gpio_set_pull_mode(pin_a, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(pin_b, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(pin_button, GPIO_PULLUP_PULLDOWN);
    gpio_set_direction(pin_a, GPIO_MODE_INPUT);
    gpio_set_direction(pin_b, GPIO_MODE_INPUT);
    gpio_set_direction(pin_button, GPIO_MODE_INPUT);
    gpio_set_intr_type(pin_a, GPIO_INTR_ANYEDGE);
    gpio_set_intr_type(pin_b, GPIO_INTR_ANYEDGE);
    gpio_isr_handler_add(pin_a, this->rotationInterruptHandler, this);
    gpio_isr_handler_add(pin_b, this->rotationInterruptHandler, this);

    debounce.down_time = 0;
    if (inverted) {
        debounce.inverted = true;
    } else {
        debounce.inverted = false;
    }

    if (debounce.inverted) debounce.history = 0xffff;

    queue = xQueueCreate(1, sizeof(rotary_encoder_event_t));
    xTaskCreate(this->buttonTask, "button_task", 2048, this, 10, NULL);

    return queue;
}

void RotaryEncoder::setRange(rotary_encoder_pos_t min, rotary_encoder_pos_t max, rotary_encoder_pos_t step, bool wrap) {
    assert(min <= max);
    assert(step >= 1);
    assert(!(wrap && step > 1));  // Wrapping with steps > 1 is confusing and not needed

    min_position = min;
    max_position = max;

    if (position < min) {
        position = min;
    } else if (position > max) {
        position = max;
    }
    step_increment = step;
    wrap_values = wrap;
}

void RotaryEncoder::setPosition(rotary_encoder_pos_t new_position) {
    assert(position >= min_position);
    assert(position <= max_position);

    position = new_position;
}

rotary_encoder_pos_t RotaryEncoder::getPosition() {
    return position;
}
