#ifndef _INCLUDE_DF_PLAYER_HPP
#define _INCLUDE_DF_PLAYER_HPP

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"

#define RECEIVE_LENGTH  10
#define SEND_LENGTH     10

#define POS_START       0
#define POS_VERSION     1
#define POS_LENGTH      2
#define POS_COMMAND     3
#define POS_FEEDBACK    4
#define POS_PARAMETER   5
#define POS_CHECKSUM    7
#define POS_END         9

#define DATA_START      0x7E
#define DATA_VERSION    0xFF
#define DATA_LENGTH     0x06
#define DATA_FEEDBACK   0x01
#define DATA_END        0xEF

typedef enum {
    DFPLAYER_NO_EVENT = 0,
    DFPLAYER_ONLINE,
    DFPLAYER_WRONG_DATA,
    DFPLAYER_PLAYER_ERROR,
    DFPLAYER_CARD_INSERTED,
    DFPLAYER_CARD_REMOVED,
    DFPLAYER_PLAY_FINISHED,
    DFPLAYER_RESPONSE_RECEIVED,
} dfplayer_event_t;

class DFPlayer {
    uart_port_t uart_port_nr;
    bool is_device_online = false;
    dfplayer_event_t last_event = DFPLAYER_NO_EVENT;
    uint16_t received_response;

    static void monitorSerialTask(void *pvParameter);
    void sendData(uint8_t command, uint16_t parameter = 0);
    void receiveData(uint16_t timeout_ms = 0);
    uint16_t calculateCRC(uint8_t *buffer);
    void decodeReceiveData(uint8_t *buffer);
    bool checkFeedbackValidityFromCommand(uint8_t command);

   public:
    bool init(uart_port_t uart_port_number, int tx_pin, int rx_pin);
    bool isDeviceOnline() { return is_device_online; }

    void playTrack(int file_number) { sendData(0x03, file_number); }
    void setVolume(uint8_t volume) { sendData(0x06, volume); }
    void loopTrack(int file_number) { sendData(0x08, file_number); }
    void stopTrack() { sendData(0x16); }

    bool checkCurrentStatus() { return checkFeedbackValidityFromCommand(0x42); }
    bool checkCurrentFileNumber() { return checkFeedbackValidityFromCommand(0x4C); }
};

#endif  // _INCLUDE_DF_PLAYER_HPP