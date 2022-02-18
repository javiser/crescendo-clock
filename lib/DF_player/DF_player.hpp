#ifndef _INCLUDE_DF_PLAYER_HPP
#define _INCLUDE_DF_PLAYER_HPP

#include "freertos/FreeRTOS.h"
#include "driver/uart.h"

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
    DFPLAYER_NO_RESPONSE = 0,
    DFPLAYER_ONLINE,
    DFPLAYER_WRONG_DATA,
    DFPLAYER_PLAYER_ERROR,
    DFPLAYER_CARD_INSERTED,
    DFPLAYER_CARD_REMOVED,
    DFPLAYER_PLAY_FINISHED,
    DFPLAYER_RESPONSE_RECEIVED,
} dfplayer_event_t;

typedef enum {
    DFPLAYER_AVAILABLE = 0,
    DFPLAYER_BUSY = 1,
    DFPLAYER_SLEEPING = 2,
    DFPLAYER_SERIAL_WRONG_DATA = 3,
    DFPLAYER_CHECKSUM_MISMATCH = 4,
    DFPLAYER_FILE_INDEX_OUT = 5,
    DFPLAYER_FILE_MISMATCH = 6,
    DFPLAYER_INVALID = 255,
} dfplayer_status_error_type_t;

class DFPlayer {
    uart_port_t _uart_port_nr;
    bool _is_device_online = false;
    dfplayer_event_t _last_event;
    uint16_t _received_response;

    static void monitorSerialTask(void *pvParameter);
    void sendData(uint8_t command, uint16_t parameter = 0);
    void receiveData(uint16_t timeout_ms = 0);
    void setEvent(dfplayer_event_t event, uint16_t response = DFPLAYER_AVAILABLE);
    uint16_t calculateCRC(uint8_t *buffer);
    void decodeReceiveData(uint8_t* buffer);
    uint16_t readFeedbackFromCommand(uint8_t command, uint16_t parameter = 0);

   public:
    bool init(uart_port_t uart_port_number, int tx_pin, int rx_pin);
    bool isDeviceOnline() { return _is_device_online; }
    dfplayer_event_t readLastEvent();
    dfplayer_status_error_type_t readErrorType(void);

    void playNextTrack() { sendData(0x01); }
    void playPreviousTrack() { sendData(0x02); }
    void playTrack(int file_number) { sendData(0x03, file_number); }
    void increaseVolume() { sendData(0x04); }
    void decreaseVolume() { sendData(0x05); }
    void setVolume(uint8_t volume) { sendData(0x06, volume); }
    void loopTrack(int file_number) { sendData(0x08, file_number); }
    void resetModule() { sendData(0x0C); }
    void resumeTrack() { sendData(0x0D); }
    void pauseTrack() { sendData(0x0E); }
    void stopTrack() { sendData(0x16); }
    
    dfplayer_status_error_type_t getCurrentStatus() { return (dfplayer_status_error_type_t)readFeedbackFromCommand(0x42); }
    uint16_t readVolume() { return readFeedbackFromCommand(0x43); }
    // TODO clean up the methods I don't need. Which will be probably a lot of them
    uint16_t getCurrentFileNumber() { return readFeedbackFromCommand(0x4C); }
    uint16_t getNumberOfFilesInFolder(int folder_number) { return readFeedbackFromCommand(0x4E, folder_number); }
    uint16_t getNumberOfFolders() { return readFeedbackFromCommand(0x4F); }
};

#endif // _INCLUDE_DF_PLAYER_HPP