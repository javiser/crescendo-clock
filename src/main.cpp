#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <wifi_time.hpp>
#include <rotary_encoder.hpp>
#include "clock_machine.hpp"
#include "clock_machine_states.hpp"
#include "clock_common.hpp"

extern "C" void app_main() {
    // Initialize NVS (needs to be done first thing in main!)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialise the rotary encoder device with the GPIOs for A and B signals
    RotaryEncoder encoder;
    QueueHandle_t event_queue = encoder.init(ROT_ENC_A_GPIO, ROT_ENC_B_GPIO, ROT_ENC_BUTTON_GPIO, ROT_ENC_BUTTON_INVERTED);

    ClockMachine machine(&encoder);  // By default a clock machine starts in state "TIME"
    rotary_encoder_event_t event;

    while (1) {
        if (xQueueReceive(event_queue, &event, 10 / portTICK_PERIOD_MS) == pdTRUE) {
            switch (event.type) {
                case ENCODER_ROTATION:
                    machine.encoderRotated(event.position, event.direction);
                    break;
                case BUTTON_SHORT_PRESS:
                    machine.buttonShortPressed();
                    break;
                case BUTTON_LONG_PRESS:
                    machine.buttonLongPressed();
                    break;
            }
        }

        // Perform whatever cyclic activities or checks are needed for this state
        machine.run();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
