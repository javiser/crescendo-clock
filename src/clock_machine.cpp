#include "nvs.h"
#include "clock_machine.hpp"
#include "clock_machine_states.hpp"
#include "esp_log.h"

ClockMachine::ClockMachine(RotaryEncoder* encoder_ref) {
    // INFO wifi stuff needs at least 1388 bytes stack, maybe more!
    // Initialize the wifi + sntp stuff
    wifi_time.init();
    wifi_time.getTime(&stored_time);

    display.init();

    if (!audio_player.init(MP3_PLAYER_UART_PORT_NUM, MP3_PLAYER_TX, MP3_PLAYER_RX)) {
        ;
        // TODO I need to handle this
        // ESP_LOGE(TAG, "There was some error initializing the MP3 player");
    }

    encoder = encoder_ref;

    // This will retrieve all stored data from NVS
    if (readNVSValues() == ESP_ERR_NVS_NOT_FOUND) {
        // This is the fault we get when we try to read data which has not yet been written in the memory. In that case we accept that and rewrite
        // the default values (all of them) into the flash. Then we try again but in this case we don't accept errors anymore and crash the SW
        writeNVSDefaultValues();
        ESP_ERROR_CHECK(readNVSValues());
    }

    state = static_cast<ClockState*>(new TimeState());
    state->enter(this);
}

esp_err_t ClockMachine::readNVSValues() {
    nvs_handle_t NVS_handle;

    esp_err_t err = nvs_open(NVS_STORAGE, NVS_READONLY, &NVS_handle);
    if (err != ESP_OK) return err;
    err = nvs_get_u8(NVS_handle, NVS_ALARM_HOUR, &alarm_time.hour);
    if (err != ESP_OK) return err;
    err = nvs_get_u8(NVS_handle, NVS_ALARM_MINUTE, &alarm_time.minute);
    if (err != ESP_OK) return err;
    size_t length = sizeof(settings);
    err = nvs_get_blob(NVS_handle, NVS_SETTINGS, &settings, &length);

    nvs_close(NVS_handle);

    return err;
}

void ClockMachine::writeNVSDefaultValues() {
    // Default some values if values not set in NVS yet
    alarm_time.hour = 7;
    alarm_time.minute = 0;
    saveAlarmTimeInNVS();
    saveSettingsInNVS();
}

void ClockMachine::saveAlarmTimeInNVS() {
    nvs_handle_t NVS_handle;

    ESP_ERROR_CHECK(nvs_open(NVS_STORAGE, NVS_READWRITE, &NVS_handle));
    ESP_ERROR_CHECK(nvs_set_u8(NVS_handle, NVS_ALARM_HOUR, alarm_time.hour));
    ESP_ERROR_CHECK(nvs_set_u8(NVS_handle, NVS_ALARM_MINUTE, alarm_time.minute));

    nvs_close(NVS_handle);
}

void ClockMachine::saveSettingsInNVS() {
    nvs_handle_t NVS_handle;

    ESP_ERROR_CHECK(nvs_open(NVS_STORAGE, NVS_READWRITE, &NVS_handle));
    ESP_ERROR_CHECK(nvs_set_blob(NVS_handle, NVS_SETTINGS, &settings, sizeof(settings)));

    nvs_close(NVS_handle);
}

void ClockMachine::setState(ClockState& newState) {
    active_timer_us = 0;
    state->exit(this);   // do stuff before we change state
    state = &newState;   // change state
    state->enter(this);  // do stuff after we change state
}

void ClockMachine::checkTimeUpdate(void) {
    clock_time_t current_time;
    wifi_time.getTime(&current_time);

    if ((current_time.hour != stored_time.hour) ||
        (current_time.minute != stored_time.minute)) {
        stored_time.hour = current_time.hour;
        stored_time.minute = current_time.minute;
        display.updateContent(D_E_TIME, &stored_time, D_A_ON);
    }
}

WifiTime* ClockMachine::getWifiTime() {
    return &wifi_time;
}

Display* ClockMachine::getDisplay() {
    return &display;
}

RotaryEncoder* ClockMachine::getEncoder() {
    return encoder;
}

DFPlayer* ClockMachine::getPlayer() {
    return &audio_player;
}

void ClockMachine::triggerTimer(uint16_t timer_ms) {
    active_timer_us = timer_ms * 1000;
    trigger_timestamp_us = esp_timer_get_time();
}

void ClockMachine::run() {
    if (active_timer_us > 0 && (esp_timer_get_time() - trigger_timestamp_us) > active_timer_us) {
        active_timer_us = 0;
        state->timerExpired(this);
    } else {
        state->run(this);
    }
}

void ClockMachine::buttonShortPressed() {
    state->buttonShortPressed(this);
}

void ClockMachine::buttonLongPressed() {
    state->buttonLongPressed(this);
}

void ClockMachine::encoderRotated(rotary_encoder_pos_t position, rotary_encoder_dir_t direction) {
    state->encoderRotated(this, position, direction);
}
