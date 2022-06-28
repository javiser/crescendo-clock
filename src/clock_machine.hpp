#ifndef _INCLUDE_CLOCK_MACHINE_HPP_
#define _INCLUDE_CLOCK_MACHINE_HPP_

#include "clock_machine_states.hpp"
#include <rotary_encoder.hpp>
#include <wifi_time.hpp>
#include <display.hpp>
#include <DF_player.hpp>

#define NVS_STORAGE         "storage"
#define NVS_ALARM_HOUR      "alarm_hour"
#define NVS_ALARM_MINUTE    "alarm_minute"
#define NVS_SETTINGS        "settings"

// Forward declaration to resolve circular dependency/include
class ClockState;

class ClockMachine {
  public:
    ClockMachine(RotaryEncoder* encoder_ref);
    // TODO So far I never needed this method, remove when it's clear that I don't need it
    // inline ClockState* getCurrentState() const {return state;}
    void saveAlarmTimeInNVS();
    void saveSettingsInNVS();
    void setState(ClockState& newState);
    void checkTimeUpdate(void);
    WifiTime* getWifiTime();
    Display* getDisplay();
    RotaryEncoder* getEncoder();
    DFPlayer* getPlayer();
    void triggerTimer(uint16_t timer_ms);
    void run();
    void buttonShortPressed();
    void buttonLongPressed();
    void encoderRotated(rotary_encoder_pos_t position, rotary_encoder_dir_t direction);
    ~ClockMachine();

    clock_time_t stored_time;
    bool is_alarm_set = false;
    clock_time_t alarm_time;
    struct {
        uint8_t crescendo_factor = 15;     // (Factor)*100 ms per volume step. If factor == 1 -> 3 seconds until maximum volume
        uint16_t snooze_time_s = 300;     // Snooze time in seconds (must be a factor of 5!)
        bool sounds_on = false;
        uint8_t melody_nr = 1;
    } settings;

  private:
    esp_err_t readNVSValues();
    void writeNVSDefaultValues();

    ClockState* state;
    WifiTime wifi_time;
    Display display;
    RotaryEncoder* encoder;
    DFPlayer audio_player;
    int64_t active_timer_us;
    int64_t trigger_timestamp_us;
};

#endif /* _INCLUDE_CLOCK_MACHINE_HPP_ */