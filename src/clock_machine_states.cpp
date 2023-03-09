#include "clock_machine_states.hpp"

#include "esp_log.h"

//--------------//
//  TIME STATE  //
//--------------//

ClockState& TimeState::getInstance() {
    static TimeState singleton;
    return singleton;
}

void TimeState::enter(ClockMachine* clock) {
    clock->getDisplay()->updateContent(D_E_TIME, &clock->stored_time, D_A_ON);
}

void TimeState::run(ClockMachine* clock) {
    bool time_has_changed = clock->checkTimeUpdate();

    if (clock->is_alarm_set &&
        (clock->stored_time.hour == clock->alarm_time.hour) &&
        (clock->stored_time.minute == clock->alarm_time.minute)) {
        clock_time_t bed_time = clock->getTimeToAlarm(clock->stored_time, clock->alarm_time);
        clock->getDisplay()->updateContent(D_E_BED_TIME, &bed_time, D_A_OFF);
        clock->setState(AlarmState::getInstance());
    }
    else if (clock->is_alarm_set && time_has_changed)
    {
        clock_time_t bed_time = clock->getTimeToAlarm(clock->stored_time, clock->alarm_time);
        clock->getDisplay()->updateContent(D_E_BED_TIME, &bed_time, D_A_ON);
    }
    
}

void TimeState::timerExpired(ClockMachine* clock) {
    clock->getDisplay()->setIncreasedBrightness(false);
}

void TimeState::buttonShortPressed(ClockMachine* clock) {
    if (clock->getDisplay()->isDisplayOn()) {
        // Invert the alarm state but only if the display was already on
        clock->is_alarm_set = !clock->is_alarm_set;
        display_action_t action = clock->is_alarm_set ? D_A_ON : D_A_OFF;        
        clock->getDisplay()->updateContent(D_E_ALARM_TIME, &clock->alarm_time, action);
        clock_time_t bed_time = clock->getTimeToAlarm(clock->stored_time, clock->alarm_time);
        clock->getDisplay()->updateContent(D_E_BED_TIME, &bed_time, action);
    }
    clock->getDisplay()->setIncreasedBrightness(true);
    clock->triggerTimer(3000);
}

void TimeState::buttonLongPressed(ClockMachine* clock) {
    clock->setState(SetAlarmState::getInstance());
}

void TimeState::encoderRotated(ClockMachine* clock, rotary_encoder_pos_t position, rotary_encoder_dir_t direction) {
    clock->getDisplay()->setIncreasedBrightness(true);
    clock->triggerTimer(3000);
}

void TimeState::exit(ClockMachine* clock) {
}

TimeState::~TimeState() {}


//---------------//
//  ALARM STATE  //
//---------------//

ClockState& AlarmState::getInstance() {
    static AlarmState singleton;
    return singleton;
}

void AlarmState::enter(ClockMachine* clock) {
    clock->getDisplay()->setMaxBrightness(true);
    alarm_volume = 5;
    crescendo_counter = 0;
    clock->getPlayer()->loopTrack(clock->settings.melody_nr);
    clock->triggerTimer(10);  // Short trigger to avoid copying code that will be in the timerExpired method
    clock->getWifiTime()->wakeUpLight();
}

void AlarmState::run(ClockMachine* clock) {
    clock->checkTimeUpdate();
}

void AlarmState::timerExpired(ClockMachine* clock) {
    crescendo_counter++;
    if (crescendo_counter == clock->settings.crescendo_factor) {
        crescendo_counter = 0;
        if (alarm_volume < 30) {
            alarm_volume++;
            clock->getPlayer()->setVolume(alarm_volume);
        }
    }
    clock->triggerTimer(500);

    display_action_t action;
    action = alarm_symbol_direction ? D_A_OFF : D_A_ON;
    alarm_symbol_direction = !alarm_symbol_direction;
    clock->getDisplay()->updateContent(D_E_ALARM_ACTIVE, NULL, action);
}

void AlarmState::buttonShortPressed(ClockMachine* clock) {
    clock->setState(SnoozeState::getInstance());
}

void AlarmState::buttonLongPressed(ClockMachine* clock) {
}

void AlarmState::encoderRotated(ClockMachine* clock, rotary_encoder_pos_t position, rotary_encoder_dir_t direction) {
    // Do some stuff in the run process
    clock->setState(TimeState::getInstance());
}

void AlarmState::exit(ClockMachine* clock) {
    // TODO There is some problem with noise after stopping the alarm. Resetting the module via HW does not help
    clock->getPlayer()->stopTrack();
    clock->getDisplay()->setMaxBrightness(false);
    clock->getDisplay()->updateContent(D_E_ALARM_TIME, &clock->alarm_time, D_A_ON);
}

AlarmState::~AlarmState() {}


//----------------//
//  SNOOZE STATE  //
//----------------//

ClockState& SnoozeState::getInstance() {
    static SnoozeState singleton;
    return singleton;
}

void SnoozeState::enter(ClockMachine* clock) {
    snooze_leaving_step = SNOOZE_WAITING;
    clock->getDisplay()->setIncreasedBrightness(true);
    clock->triggerTimer(3000);  // To show the time 3 seconds after snoozing
    snooze_start_timer_s = (int64_t)(esp_timer_get_time() / 1000000);
    remaining_snooze_time_s = clock->settings.snooze_time_s;
}

void SnoozeState::run(ClockMachine* clock) {
    int64_t now_s = (int64_t)(esp_timer_get_time() / 1000000);
    uint16_t snooze_time_left_s = (uint16_t)(clock->settings.snooze_time_s - now_s + snooze_start_timer_s);
    if (snooze_time_left_s < remaining_snooze_time_s) {
        if (snooze_time_left_s == 0) {
            clock->setState(AlarmState::getInstance());
        } else {
            remaining_snooze_time_s = snooze_time_left_s;
            clock->getDisplay()->updateContent(D_E_SNOOZE_TIME, &snooze_time_left_s, D_A_ON);
        }
    }

    clock->checkTimeUpdate();
}

void SnoozeState::timerExpired(ClockMachine* clock) {
    clock->getDisplay()->setIncreasedBrightness(false);
    snooze_leaving_step = SNOOZE_WAITING;
    clock->getDisplay()->updateContent(D_E_SNOOZE_CANCEL, NULL, D_A_OFF);
    //ESP_LOGI("S", "Back to the start position");
}

void SnoozeState::buttonShortPressed(ClockMachine* clock) {
    clock->getDisplay()->setIncreasedBrightness(true);
    clock->triggerTimer(3000);
}

void SnoozeState::buttonLongPressed(ClockMachine* clock) {
    if (snooze_leaving_step == SNOOZE_FIRST_ROTATION) {
        snooze_leaving_step = SNOOZE_LONG_PRESS;
        clock->getDisplay()->updateContent(D_E_SNOOZE_CANCEL, NULL, D_A_TWO_BARS);
        //Yes! Now a rotation in the other direction!!!
    }
    // No matter in what state are we, 3 seconds more light.
    clock->getDisplay()->setIncreasedBrightness(true);
    clock->triggerTimer(3000);
}

void SnoozeState::encoderRotated(ClockMachine* clock, rotary_encoder_pos_t position, rotary_encoder_dir_t direction) {
    if (snooze_leaving_step == SNOOZE_WAITING) {
        snooze_leaving_step = SNOOZE_FIRST_ROTATION;
        first_rotation_dir = direction;
        clock->getDisplay()->updateContent(D_E_SNOOZE_CANCEL, NULL, D_A_ONE_BAR);
        // Now a long press...
    } else if (snooze_leaving_step == SNOOZE_LONG_PRESS and direction != first_rotation_dir) {
        // Yes! Snooze cancellation sequence complete!
        clock->getDisplay()->updateContent(D_E_SNOOZE_CANCEL, NULL, D_A_OFF);
        clock->is_alarm_set = false;
        clock->getDisplay()->updateContent(D_E_ALARM_TIME, &clock->alarm_time, D_A_OFF);
        clock->getWifiTime()->stopWakeUpLight();
        clock->setState(TimeState::getInstance());
    }

    // Otherwise we are in the first rotation step, keep the window alive.
    // In any case, even if we leave the snooze state, 3 seconds more light
    clock->getDisplay()->setIncreasedBrightness(true);
    clock->triggerTimer(3000);
}

void SnoozeState::exit(ClockMachine* clock) {
    clock->getDisplay()->updateContent(D_E_SNOOZE_TIME, NULL, D_A_OFF);
}

SnoozeState::~SnoozeState() {}

//-------------------//
//  SET ALARM STATE  //
//-------------------//

ClockState& SetAlarmState::getInstance() {
    static SetAlarmState singleton;
    return singleton;
}

void SetAlarmState::enter(ClockMachine* clock) {
    clock->getDisplay()->setIncreasedBrightness(true);
    clock->getDisplay()->updateContent(D_E_ALARM_TIME, &clock->alarm_time, D_A_HIDE_HOURS);
    hours_hidden = true;
    setting_minutes = false;  // We begin with the hours
    clock->triggerTimer(500);
    clock->getEncoder()->setRange(0, 23, true, true);
    clock->getEncoder()->setPosition(clock->alarm_time.hour);
    original_alarm_time.hour = clock->alarm_time.hour;
    original_alarm_time.minute = clock->alarm_time.minute;
}

void SetAlarmState::run(ClockMachine* clock) {
    clock->checkTimeUpdate();
}

void SetAlarmState::timerExpired(ClockMachine* clock) {
    display_action_t action;

    if (!setting_minutes) {
        // Setting the hours yet
        action = hours_hidden ? D_A_ON : D_A_HIDE_HOURS;
        hours_hidden = !hours_hidden;
    } else {
        // We are setting the minutes now
        action = minutes_hidden ? D_A_ON : D_A_HIDE_MINUTES;
        minutes_hidden = !minutes_hidden;
    }
    clock->getDisplay()->updateContent(D_E_ALARM_TIME, &clock->alarm_time, action);
    clock->triggerTimer(500);
}

void SetAlarmState::buttonShortPressed(ClockMachine* clock) {
    if (!setting_minutes) {
        clock->getDisplay()->updateContent(D_E_ALARM_TIME, &clock->alarm_time, D_A_HIDE_MINUTES);
        minutes_hidden = true;
        setting_minutes = true;  // It's turn for the minutes now
        clock->triggerTimer(500);
        clock->getEncoder()->setRange(0, 59, true, true);
        clock->getEncoder()->setPosition(clock->alarm_time.minute);
    } else {
        clock->setState(TimeState::getInstance());
    }
}

void SetAlarmState::buttonLongPressed(ClockMachine* clock) {
    // Cancel the alarm setting and restore the original times
    clock->alarm_time.hour = original_alarm_time.hour;
    clock->alarm_time.minute = original_alarm_time.minute;
    clock->setState(TimeState::getInstance());
}

void SetAlarmState::encoderRotated(ClockMachine* clock, rotary_encoder_pos_t position, rotary_encoder_dir_t direction) {
    if (!setting_minutes) {
        clock->alarm_time.hour = clock->getEncoder()->getPosition();
        hours_hidden = false;

    } else {
        clock->alarm_time.minute = clock->getEncoder()->getPosition();
        minutes_hidden = false;
    }
    clock->getDisplay()->updateContent(D_E_ALARM_TIME, &clock->alarm_time, D_A_ON);
    clock_time_t bed_time = clock->getTimeToAlarm(clock->stored_time, clock->alarm_time);
    clock->getDisplay()->updateContent(D_E_BED_TIME, &bed_time, D_A_ON);

    clock->triggerTimer(500);
}

void SetAlarmState::exit(ClockMachine* clock) {
    clock->getDisplay()->updateContent(D_E_ALARM_TIME, &clock->alarm_time, D_A_ON);
    clock->is_alarm_set = true;  // After setting the new alarm time alarm is set
    clock->saveAlarmTimeInNVS();

    if (clock->settings.sounds_on) {
        clock->getPlayer()->setVolume(10);
        clock->getPlayer()->playTrack(CONFIRMATION_TRACK);
    }
    clock->triggerTimer(3000);  // To show the new alarm time at least 3 seconds
}

SetAlarmState::~SetAlarmState() {}
