
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "driver/ledc.h"
#include <display.hpp>
// TODO This is a temporary custom font
#include <Antonio_SemiBold75pt7b.h>

#include "esp_log.h"
static const char *TAG = "display";

void Display::monitorBrightnessTask(void *pvParameter) {
    Display *pThis = (Display *)pvParameter;
    bool event;
    while (1) {
        xQueueReceive(pThis->queue, &event, 1000 / portTICK_PERIOD_MS);
        pThis->controlBrightness();
    }
}

void Display::init(void) {
    lcd.init();
    lcd.setRotation(0);
    lcd.setColorDepth(16);

    // ADC1 config for light sensor
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));    
    adc_oneshot_chan_cfg_t config = {
        .atten = LIGHT_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, LIGHT_ADC_CHANNEL, &config));

    // TODO When I didn't have debug information in controlBrightness I could set the stack to 768 (but not 512)
    xTaskCreate(this->monitorBrightnessTask, "monitor_brightness_task", 2048, this, 1, NULL);
    queue = xQueueCreate(1, sizeof(bool));
}

void Display::updateContent(display_element_t element, void *value, display_action_t action) {
    // TODO I need a display design to arrange everything properly
    switch (element) {
        case D_E_TIME:
            char time_buf[8];
            sprintf(time_buf, "%02d:%02d", static_cast<clock_time_t *>(value)->hour, static_cast<clock_time_t *>(value)->minute);
            lcd.setTextDatum(top_center);

            switch (action) {
                case D_A_ON:
                    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
                    break;
                default:
                    break;
            }
            lcd.drawString(time_buf, lcd.width() / 2, 10, &Antonio_SemiBold75pt7b);
            break;

        case D_E_ALARM_TIME:
            char alarm_buf[8];
            sprintf(alarm_buf, "%02d:%02d", static_cast<clock_time_t *>(value)->hour, static_cast<clock_time_t *>(value)->minute);
            lcd.setTextColor(TFT_WHITE, TFT_BLACK);  // Normal case
			lcd.setTextDatum(middle_center);
            switch (action) {
                case D_A_OFF:
                    lcd.setTextColor(TFT_BLACK, TFT_BLACK);
                    break;
                case D_A_HIDE_HOURS:
                    alarm_buf[0] = alarm_buf[1] = ' ';
                    break;
                case D_A_HIDE_MINUTES:
                    alarm_buf[3] = alarm_buf[4] = ' ';
                    break;
                default:
                    break;
            }
            lcd.drawString(alarm_buf, 60, 205, &FreeMono18pt7b);
            break;

        case D_E_SNOOZE_TIME:
            char snooze_buf[8];
            lcd.setTextDatum(middle_center);
            lcd.setTextColor(TFT_GREEN, TFT_BLACK);
            switch (action) {
                case D_A_OFF:
                    sprintf(snooze_buf, "     ");
                    break;
                case D_A_ON:
                    uint8_t minutes;
                    uint8_t seconds;
                    uint16_t remaining_seconds;
                    remaining_seconds = *(static_cast<uint16_t *>(value));
                    minutes = remaining_seconds / 60;
                    seconds = remaining_seconds % 60;
                    sprintf(snooze_buf, "%02d:%02d", minutes, seconds);
                    break;
                default:
                    break;
            }
            lcd.drawString(snooze_buf, 60, 170, &FreeMono18pt7b);
            break;

        default:
            break;
    }
}

void Display::controlBrightness(void) {
    int adc_raw;
    uint16_t ambient_light = 0;

    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, LIGHT_ADC_CHANNEL, &adc_raw));
    ambient_light = (uint16_t)adc_raw;

    if (ambient_light == 0) {
        // We could not read the ambient light, we try again in the next iteration
        return;
    }

    if (max_brightness_requested) {
        setBrightness(DISPLAY_BRIGHTNESS_LEVELS_NR);
    } else {
        // Adjust the brightness level if necessary
        if ((display_brightness_level > 0) &&
            (ambient_light < display_light_thd_down[display_brightness_level - 1])) {
            display_brightness_level--;
        } else if ((display_brightness_level < DISPLAY_BRIGHTNESS_LEVELS_NR) &&
                   (ambient_light > display_light_thd_up[display_brightness_level])) {
            display_brightness_level++;
        }
        if (increased_brightness_requested) {
            setBrightness(display_brightness_level + 1);
        } else {
            setBrightness(display_brightness_level);
        }
    }
/*
    // TODO For debugging purposes only, I don't care here about sprites or similar
    char light_buf[10];
    sprintf(light_buf, "%03d (%d)", ambient_light, display_brightness_level);
    ESP_LOGI(TAG, "Ambient light = %d", ambient_light);
    lcd.setTextDatum(bottom_right);
    lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    lcd.drawString(light_buf, lcd.width() - 5, lcd.height() - 5, &FreeSans12pt7b);
	*/
}

void Display::setBrightness(uint8_t brightness_level) {
    // In case we have an increased brightness level, we saturate it to the last level
    if (brightness_level > DISPLAY_BRIGHTNESS_LEVELS_NR)
        brightness_level = DISPLAY_BRIGHTNESS_LEVELS_NR;

    lcd.setBrightness(display_light_brightness[brightness_level]);
}

void Display::setMaxBrightness(bool request_max_brightness) {
    max_brightness_requested = request_max_brightness;
    increased_brightness_requested = false;
    // This is to trigger an immediate change of brightness in monitorBrightnessTask
    bool queue_event = true;
    xQueueSend(queue, &queue_event, portMAX_DELAY);
}

void Display::setIncreasedBrightness(bool request_inc_brightness) {
     // This is to trigger an immediate change of brightness in monitorBrightnessTask
    if (increased_brightness_requested != request_inc_brightness) {
        bool queue_event = true;
        xQueueSend(queue, &queue_event, portMAX_DELAY);
    }
    increased_brightness_requested = request_inc_brightness;
    max_brightness_requested = false;
}

bool Display::isDisplayOn(void) {
    return (display_brightness_level > 0 || increased_brightness_requested);
}

void Display::setLEDsDuties(uint8_t red_duty, uint8_t green_duty) {
    // TODO There is a bug in esp-idf: 255 does not mean 100% PWM, 256 is.
    // Since we are using inverted logic, with input = 0 -> 255-0 = 255 light is slightly on
    uint32_t ledc_duty_red = (red_duty == 0) ? 256 : (uint32_t)(255 - red_duty);
    uint32_t ledc_duty_green = (green_duty == 0) ? 256 : (uint32_t)(255 - green_duty);
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_RED, ledc_duty_red));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_GREEN, ledc_duty_green));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_RED));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_GREEN));
}

// TODO This is a temporary public function, I still don't know if I really need it like this
void Display::fadeEffect(display_led_t led, uint16_t fade_time_on_ms, uint16_t fade_time_off_ms) {
    ledc_channel_t channel = (led == D_LED_GREEN) ? LEDC_CHANNEL_GREEN : LEDC_CHANNEL_RED;
    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, channel, 0, fade_time_on_ms);
    ledc_fade_start(LEDC_LOW_SPEED_MODE, channel, LEDC_FADE_NO_WAIT);
    // TODO This is a blocking thing, in the future I will need to do it in a separate task or similar
    vTaskDelay(fade_time_on_ms / portTICK_PERIOD_MS);
    // TODO Here the hard coded 256, see comment in function setLEDsDuties
    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, channel, 256, fade_time_off_ms);
    ledc_fade_start(LEDC_LOW_SPEED_MODE, channel, LEDC_FADE_NO_WAIT);
}