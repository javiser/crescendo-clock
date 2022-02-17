#include "DF_player.hpp"
#include "freertos/task.h"

#ifdef _DEBUG
#include "esp_log.h"
static const char *TAG = "player_lib";
#endif

#define BUF_SIZE (2048)

void DFPlayer::monitorSerialTask(void *pvParameter) {
    DFPlayer *pThis = (DFPlayer *)pvParameter;
    while (1) {
        pThis->receiveData();
    }
}

// TODO there is a lot of blocking going on. For begin() it's fine but for the send commands? Not so much
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
    _uart_port_nr = uart_port_number;

    ESP_ERROR_CHECK(uart_driver_install(_uart_port_nr, BUF_SIZE, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(_uart_port_nr, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(_uart_port_nr, pin_tx, pin_rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    resetModule();
    receiveData(200);  // First get a reply to the reset
    receiveData(200);  // For some reason, we get a single byte after reset which needs to be ignored
    receiveData(200);  // Now we get some information about the player to return the begin status

    // Spawn a task to monitor the incoming, unexpected serial messages
    xTaskCreate(this->monitorSerialTask, "monitor_serial_task", 2048, this, 10, NULL);

    return (_last_event == DFPLAYER_ONLINE);
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
    number_of_bytes = uart_read_bytes(_uart_port_nr, rcvd_buffer, RECEIVE_LENGTH, ticks_to_wait);
    if (number_of_bytes == -1) {
        ;  // TODO when we get to this point everything goes crazy, we need to catch this somehow
        #ifdef _DEBUG
        ESP_LOGE(TAG, "Error receiving bytes");
        #endif
    } else if (number_of_bytes > 0) {
        if (number_of_bytes < RECEIVE_LENGTH) {
            // For some reason, we get a single byte after reset which needs to be ignored
            ;
            #ifdef _DEBUG
            ESP_LOGI(TAG, "Received only %d bytes, ignoring", number_of_bytes);
            #endif
        } else {
            // Now check the bytes
            if ((rcvd_buffer[POS_START] != DATA_START) || (rcvd_buffer[POS_VERSION] != DATA_VERSION) ||
                (rcvd_buffer[POS_LENGTH] != DATA_LENGTH) || (rcvd_buffer[POS_END] != DATA_END)) {
                setEvent(DFPLAYER_WRONG_DATA);
                return;
            }
            if (calculateCRC(rcvd_buffer) == (rcvd_buffer[POS_CHECKSUM] << 8) + (rcvd_buffer[POS_CHECKSUM + 1])) {
                decodeReceiveData(rcvd_buffer);
            } else {
                setEvent(DFPLAYER_WRONG_DATA);
            }
        }
    } 
    #ifdef _DEBUG
    else {
        ESP_LOGE(TAG, "Expecting to get some bytes, but nothing there");
    }
    #endif
}

void DFPlayer::sendData(uint8_t command, uint16_t parameter) {
    uint8_t data_buffer[SEND_LENGTH] = {DATA_START, DATA_VERSION, DATA_LENGTH, 0x00, DATA_FEEDBACK, 0x00, 0x00, 0x00, 0x00, DATA_END};

    data_buffer[POS_COMMAND] = command;
    data_buffer[POS_PARAMETER] = (uint8_t)parameter >> 8;
    data_buffer[POS_PARAMETER + 1] = (uint8_t)parameter;

    uint16_t data_CRC = calculateCRC(data_buffer);
    data_buffer[POS_CHECKSUM] = (uint8_t)(data_CRC >> 8);
    data_buffer[POS_CHECKSUM + 1] = (uint8_t)data_CRC;

    uart_write_bytes(_uart_port_nr, (const char *)data_buffer, SEND_LENGTH);
    vTaskDelay(100 / portTICK_PERIOD_MS);
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
    uint16_t parameter = (rcvd_buffer[POS_PARAMETER] << 8) + (rcvd_buffer[POS_PARAMETER + 1]);
    switch (command) {
        case 0x3D:
            setEvent(DFPLAYER_PLAY_FINISHED);
            break;
        case 0x3F:
            // We assume that we are only using the SD card
            setEvent(DFPLAYER_ONLINE);
            _is_device_online = true;
            break;
        case 0x3A:
            // We assume that we are only using the SD card
            setEvent(DFPLAYER_CARD_INSERTED);
            _is_device_online = true;
            break;
        case 0x3B:
            // We assume that we are only using the SD card
            setEvent(DFPLAYER_CARD_REMOVED);
            _is_device_online = false;
            break;
        case 0x40:
            setEvent(DFPLAYER_PLAYER_ERROR, (dfplayer_status_error_type_t)parameter);
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
            setEvent(DFPLAYER_RESPONSE_RECEIVED);
            _received_response = parameter;
            break;
        default:
            // Something else happened, we just ignore this
            break;
    }
}

void DFPlayer::setEvent(dfplayer_event_t event, uint16_t response) {
    _last_event = event;
    _received_response = response;

    #ifdef _DEBUG
    if (event == DFPLAYER_PLAYER_ERROR)
        ESP_LOGE(TAG, "Player error! Error type = %d", response);
    else {
        switch (event) {
            case DFPLAYER_ONLINE:
                ESP_LOGI(TAG, "Status: online");
                break;
            case DFPLAYER_WRONG_DATA:
                ESP_LOGE(TAG, "Status: wrong stack");
                break;
            case DFPLAYER_CARD_INSERTED:
                ESP_LOGI(TAG, "Status: card inserted");
                break;
            case DFPLAYER_CARD_REMOVED:
                ESP_LOGI(TAG, "Status: card removed");
                break;
            case DFPLAYER_PLAY_FINISHED:
                ESP_LOGI(TAG, "Status: play finished");
                break;
            case DFPLAYER_RESPONSE_RECEIVED:
                ESP_LOGI(TAG, "Status: response received");
                break;
            default:
                ESP_LOGI(TAG, "Some other status %d", event);
                break;
        }
    }
    #endif
}

dfplayer_event_t DFPlayer::readLastEvent() {
    return _last_event;
}

dfplayer_status_error_type_t DFPlayer::readErrorType(void) {
    return (dfplayer_status_error_type_t)_received_response;
}

uint16_t DFPlayer::readFeedbackFromCommand(uint8_t command, uint16_t parameter) {
    sendData(command, parameter);
    if (_last_event == DFPLAYER_RESPONSE_RECEIVED)
        return _received_response;
    else
        return DFPLAYER_INVALID;
}
