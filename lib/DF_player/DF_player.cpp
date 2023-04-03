#include "DF_player.hpp"

#include "esp_log.h"
#include "freertos/task.h"
static const char *TAG = "df_player";

#define BUF_SIZE (2048)

void DFPlayer::monitorSerialTask(void *pvParameter) {
    DFPlayer *pThis = (DFPlayer *)pvParameter;
    while (1) {
        pThis->receiveData();
    }
}

// There is a lot of blocking going on. For begin() it's fine but for the send commands? Not so much.
// However for our purposes this will just do
bool DFPlayer::init(uart_port_t uart_port_number, int pin_tx, int pin_rx) {
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,  // Some dummy value to avoid compiler warning, not needed
        .source_clk = UART_SCLK_APB,
    };
    uart_port_nr = uart_port_number;

    ESP_ERROR_CHECK(uart_driver_install(uart_port_nr, BUF_SIZE, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(uart_port_nr, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_port_nr, pin_tx, pin_rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // Spawn a task to monitor the incoming serial messages
    xTaskCreate(this->monitorSerialTask, "monitor_serial_task", 2048, this, 10, NULL);

    // Small pause before we begin with the requests
    vTaskDelay(200 / portTICK_PERIOD_MS);
    return (checkCurrentStatus());
}

void DFPlayer::receiveData(uint16_t timeout_ms) {
    TickType_t ticks_to_wait;
    if (timeout_ms == 0)
        ticks_to_wait = portMAX_DELAY;
    else
        ticks_to_wait = timeout_ms / portTICK_PERIOD_MS;

    // Now get the reply
    int number_of_bytes = 0;
    uint8_t rcvd_buffer[RECEIVE_LENGTH];
    number_of_bytes = uart_read_bytes(uart_port_nr, rcvd_buffer, RECEIVE_LENGTH, ticks_to_wait);
    if (number_of_bytes == -1) {
        ;  // If we come to this point, everything goes crazy. This made problems in the past
        ESP_LOGE(TAG, "Error receiving bytes");
    } else if (number_of_bytes > 0) {
        if (number_of_bytes < RECEIVE_LENGTH) {
            // For some reason, we get a single byte after reset which needs to be ignored
            ;
        } else {
            // Now check the bytes
            if ((rcvd_buffer[POS_START] != DATA_START) || (rcvd_buffer[POS_VERSION] != DATA_VERSION) ||
                (rcvd_buffer[POS_LENGTH] != DATA_LENGTH) || (rcvd_buffer[POS_END] != DATA_END)) {
                last_event = DFPLAYER_WRONG_DATA;
                ESP_LOGE(TAG, "New event: wrong data");
                return;
            }
            if (calculateCRC(rcvd_buffer) == (rcvd_buffer[POS_CHECKSUM] << 8) + (rcvd_buffer[POS_CHECKSUM + 1])) {
                decodeReceiveData(rcvd_buffer);
            } else {
                last_event = DFPLAYER_WRONG_DATA;
                ESP_LOGE(TAG, "New event: wrong data");
            }
        }
    } else {
        ESP_LOGE(TAG, "Expecting to get some bytes, but nothing there");
    }
}

void DFPlayer::sendData(uint8_t command, uint16_t parameter) {
    // We reset the last event
    last_event = DFPLAYER_NO_EVENT;
    uint8_t data_buffer[SEND_LENGTH] = {DATA_START, DATA_VERSION, DATA_LENGTH, 0x00, DATA_FEEDBACK, 0x00, 0x00, 0x00, 0x00, DATA_END};
    data_buffer[POS_COMMAND] = command;
    data_buffer[POS_PARAMETER] = (uint8_t)parameter >> 8;
    data_buffer[POS_PARAMETER + 1] = (uint8_t)parameter;

    uint16_t data_CRC = calculateCRC(data_buffer);
    data_buffer[POS_CHECKSUM] = (uint8_t)(data_CRC >> 8);
    data_buffer[POS_CHECKSUM + 1] = (uint8_t)data_CRC;

    uart_write_bytes(uart_port_nr, (const char *)data_buffer, SEND_LENGTH);
    vTaskDelay(200 / portTICK_PERIOD_MS);
}

uint16_t DFPlayer::calculateCRC(uint8_t *buffer) {
    uint16_t total = 0;
    for (int index = POS_VERSION; index < POS_CHECKSUM; index++) {
        total += buffer[index];
    }
    return -total;
}

void DFPlayer::decodeReceiveData(uint8_t *rcvd_buffer) {
    uint8_t command = rcvd_buffer[POS_COMMAND];
    uint16_t parameter = static_cast<uint16_t>(rcvd_buffer[POS_PARAMETER] << 8) + (rcvd_buffer[POS_PARAMETER + 1]);
    switch (command) {
        case 0x3D:
            last_event = DFPLAYER_PLAY_FINISHED;
            ESP_LOGI(TAG, "New event: play finished");
            break;
        case 0x3F:
            // We assume that we are only using the SD card
            last_event = DFPLAYER_ONLINE;
            ESP_LOGI(TAG, "New event: player online");
            is_device_online = true;
            break;
        case 0x3A:
            // We assume that we are only using the SD card
            last_event = DFPLAYER_CARD_INSERTED;
            ESP_LOGI(TAG, "New event: card inserted");
            // This does not automatically mean online device. But we will assume it is for the sake of simplicity
            is_device_online = true;
            break;
        case 0x3B:
            // We assume that we are only using the SD card
            last_event = DFPLAYER_CARD_REMOVED;
            is_device_online = false;
            ESP_LOGI(TAG, "New event: card removed");
            break;
        case 0x40:
            last_event = DFPLAYER_PLAYER_ERROR;
            is_device_online = false;
            ESP_LOGE(TAG, "New event: player error, parameter = %d", parameter);
            break;
        case 0x41:
            // Command reply, we can ignore this
            return;
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x47:
        case 0x4B:
        case 0x4C:
        case 0x4D:
        case 0x4E:
        case 0x4F:
            last_event = DFPLAYER_RESPONSE_RECEIVED;
            received_response = parameter;
            is_device_online = true;
            ESP_LOGI(TAG, "New event: response received = %d", parameter);
            break;
        default:
            ESP_LOGE(TAG, "Unknown event with ID %X", command);
            // Something else happened, we just ignore this
            break;
    }
}

bool DFPlayer::checkFeedbackValidityFromCommand(uint8_t command) {
    sendData(command);
    return (last_event == DFPLAYER_RESPONSE_RECEIVED);
}
