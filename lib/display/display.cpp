
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "driver/ledc.h"
#include <display.hpp>
#include <Antonio_SemiBold75pt7b.h>
#include <Antonio_Regular26pt7b.h>
#include <Antonio_Light16pt7b.h>

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

// TODO Overload this function for the case we don't need to pass any parameter for the value
void Display::updateContent(display_element_t element, void *value, display_action_t action) {
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
            lcd.drawString(time_buf, (lcd.width() / 2) + 5, 10, &Antonio_SemiBold75pt7b);
            break;

        case D_E_ALARM_TIME:
            char alarm_buf[8];
            sprintf(alarm_buf, "%02d:%02d", static_cast<clock_time_t *>(value)->hour, static_cast<clock_time_t *>(value)->minute);
            char alarm_symbol_buf[2];
            sprintf(alarm_symbol_buf, DISPLAY_SYMBOL_ALARM_ON);
            lcd.setTextColor(TFT_WHITE, TFT_BLACK);  // Normal case
			lcd.setTextDatum(middle_center);
            switch (action) {
                case D_A_OFF:
                    lcd.setTextColor(TFT_DARKGRAY, TFT_BLACK);
                    sprintf(alarm_symbol_buf, DISPLAY_SYMBOL_ALARM_OFF);
                    sprintf(alarm_buf, "     ");
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
            lcd.drawString(alarm_symbol_buf, 35, 205, &Antonio_Regular26pt7b);
            lcd.drawString(alarm_buf, 100, 200, &Antonio_Regular26pt7b);
            break;

        case D_E_ALARM_ACTIVE:
            char alarm_active_symbol_buf[2];
            lcd.setTextColor(TFT_WHITE, TFT_BLACK);  // Normal case
			lcd.setTextDatum(middle_center);
            switch (action) {
                case D_A_OFF:
                    sprintf(alarm_active_symbol_buf, DISPLAY_SYMBOL_ALARM_L);
                    break;
                case D_A_ON:
                    sprintf(alarm_active_symbol_buf, DISPLAY_SYMBOL_ALARM_R);
                    break;
                default:
                    break;
            }
            lcd.drawString(alarm_active_symbol_buf, 35, 205, &Antonio_Regular26pt7b);
            break;

        case D_E_BED_TIME:
            char bed_time_buf[8];
            sprintf(bed_time_buf, "%01d:%02d", static_cast<clock_time_t *>(value)->hour, static_cast<clock_time_t *>(value)->minute);
            char bed_time_symbol_buf[3];
            sprintf(bed_time_symbol_buf, DISPLAY_SYMBOL_BED);
            lcd.setTextColor(TFT_LIGHTGRAY, TFT_BLACK);  // Normal case
			lcd.setTextDatum(middle_center);
            // We show the remaining bed time only when less than 9 hours
            if (static_cast<clock_time_t *>(value)->hour >= 9)
                action = D_A_OFF;

            switch (action) {
                case D_A_OFF:
                    sprintf(bed_time_buf, "    ");
                    sprintf(bed_time_symbol_buf, "  ");
                    break;
                default:
                    break;
            }
            lcd.drawString(bed_time_symbol_buf, 180, 205, &Antonio_Regular26pt7b);
            lcd.drawString(bed_time_buf, 235, 200, &Antonio_Regular26pt7b);
            break;

        case D_E_SNOOZE_TIME:
            char snooze_buf[8];
            lcd.setTextDatum(middle_center);
            lcd.setTextColor(TFT_ORANGE, TFT_BLACK);
            switch (action) {
                case D_A_OFF:
                    sprintf(snooze_buf, "     ");
                    lcd.drawString("  ", 175, 205, &Antonio_Regular26pt7b);
                    break;
                case D_A_ON:
                    uint8_t minutes;
                    uint8_t seconds;
                    uint16_t remaining_seconds;
                    remaining_seconds = *(static_cast<uint16_t *>(value));
                    minutes = remaining_seconds / 60;
                    seconds = remaining_seconds % 60;
                    sprintf(snooze_buf, "%01d:%02d", minutes, seconds);
                    lcd.drawString(DISPLAY_SYMBOL_SNOOZE, 175, 205, &Antonio_Regular26pt7b);
                    break;
                default:
                    break;
            }
            lcd.drawString(snooze_buf, 230, 200, &Antonio_Regular26pt7b);
            break;

        case D_E_SNOOZE_CANCEL:
            switch (action) {
                case D_A_OFF:
                    // Clear all the bars
                    lcd.setColor(TFT_BLACK);
                    lcd.fillRect(160, 160, 75, 5);
                    break;
                case D_A_ONE_BAR:
                    // Draw only the first bar
                    lcd.setColor(TFT_ORANGE);
                    lcd.fillRect(160, 160, 35, 5);
                    break;
                case D_A_TWO_BARS:
                    // Draw only the second bar
                    lcd.setColor(TFT_ORANGE);
                    lcd.fillRect(200, 160, 35, 5);
                    break;
                default:
                    // There is no "case 3" where all 3 bars are shown
                    break;
            }
            break;

        case D_E_WIFI_STATUS:
            lcd.setTextDatum(middle_center);
            switch (action) {
                case D_A_OFF:
                    lcd.setTextColor(TFT_RED, TFT_BLACK);
                    lcd.drawString(DISPLAY_SYMBOL_WIFI_OFF, 295, 170, &Antonio_Regular26pt7b);
                    break;
                case D_A_ON:
                    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
                    lcd.drawString(DISPLAY_SYMBOL_WIFI_ON, 295, 170, &Antonio_Regular26pt7b);
                    break;
                default:
                    break;
            }
            break;

        case D_E_MQTT_STATUS:
            lcd.setTextDatum(middle_center);
            switch (action) {
                case D_A_OFF:
                    lcd.setTextColor(TFT_RED, TFT_BLACK);
                    lcd.drawString(DISPLAY_SYMBOL_MQTT_OFF, 295, 205, &Antonio_Regular26pt7b);
                    break;
                case D_A_ON:
                    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
                    lcd.drawString(DISPLAY_SYMBOL_MQTT_ON, 295, 205, &Antonio_Regular26pt7b);
                    break;
                default:
                    break;
            }
            break;

        case D_E_WIFI_SETTING:
            lcd.setTextDatum(middle_center);
            lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
            switch (action) {
                case D_A_ON:
                    lcd.drawString(DISPLAY_SYMBOL_WIFI_COG, 295, 170, &Antonio_Regular26pt7b);
                    lcd.drawString("PRESS", 220, 175, &Antonio_Light16pt7b);
                    lcd.drawString("WPS", 220, 210, &Antonio_Light16pt7b);
                    break;
                default:
                    lcd.drawString("  ", 295, 170, &Antonio_Regular26pt7b);
                    lcd.setColor(TFT_BLACK);
                    lcd.fillRect(180, 150, 90, 80);
                    break;
            }
            break;

        case D_E_AUDIO:
            lcd.setTextDatum(middle_center);
            lcd.setTextColor(TFT_RED, TFT_BLACK);
            switch (action) {
                case D_A_OFF:
                    lcd.drawString(DISPLAY_SYMBOL_AUDIO_OFF, 35, 170, &Antonio_Regular26pt7b);
                    break;
                default:
                    lcd.drawString("  ", 35, 170, &Antonio_Regular26pt7b);
                    break;
            }
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
