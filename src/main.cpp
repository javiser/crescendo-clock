#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <wifi_time.hpp>
#include <rotary_encoder.hpp>
#include "clock_machine.hpp"
#include "clock_machine_states.hpp"
#include "clock_common.hpp"

static const char *TAG = "main";

extern "C" void app_main() {
    // INFO NVS initialization needs 560 bytes stack!
    // Initialize NVS (needs to be done first thing in main!)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // INFO rotary encoder 96 stack bytes
    // Initialise the rotary encoder device with the GPIOs for A and B signals
    RotaryEncoder encoder;
    QueueHandle_t event_queue = encoder.init(ROT_ENC_A_GPIO, ROT_ENC_B_GPIO, ROT_ENC_BUTTON_GPIO, ROT_ENC_BUTTON_INVERTED);

    // INFO machine stack size = 1632 bytes
    ClockMachine machine(&encoder);  // By default a clock machine starts in state "TIME"
    rotary_encoder_event_t event;

    while (1) {
        if (xQueueReceive(event_queue, &event, 10 / portTICK_PERIOD_MS) == pdTRUE) {
            switch (event.type) {
                case ENCODER_ROTATION:
                    machine.encoderRotated(event.position, event.direction);
                    break;
                case BUTTON_SHORT_PRESS:
                    ESP_LOGI(TAG, "Short press");
                    //ESP_LOGI("main", "Stack = %d", uxTaskGetStackHighWaterMark(NULL));
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
