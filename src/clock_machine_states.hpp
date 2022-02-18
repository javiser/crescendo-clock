#ifndef _INCLUDE_CLOCK_MACHINE_STATES_HPP_
#define _INCLUDE_CLOCK_MACHINE_STATES_HPP_

#include <rotary_encoder.hpp>

#include "clock_machine.hpp"

#define NUMBER_MELODIES     3
#define CONFIRMATION_TRACK  101

// Forward declaration to resolve circular dependency/include
class ClockMachine;

class ClockState {
   public:
    virtual void enter(ClockMachine* clock) = 0;
    virtual void run(ClockMachine* clock) = 0;
    virtual void timerExpired(ClockMachine* clock) = 0;
    virtual void buttonShortPressed(ClockMachine* clock) = 0;
    virtual void buttonLongPressed(ClockMachine* clock) = 0;
    virtual void encoderRotated(ClockMachine* clock, rotary_encoder_pos_t position, rotary_encoder_dir_t direction = DIR_RIGHT) = 0;
    virtual void exit(ClockMachine* clock) = 0;
    virtual ~ClockState() {}
};

class TimeState : public ClockState {
   public:
    virtual void enter(ClockMachine* clock);
    virtual void run(ClockMachine* clock);
    virtual void timerExpired(ClockMachine* clock);
    virtual void buttonShortPressed(ClockMachine* clock);
    virtual void buttonLongPressed(ClockMachine* clock);
    virtual void encoderRotated(ClockMachine* clock, rotary_encoder_pos_t position, rotary_encoder_dir_t direction = DIR_RIGHT);
    virtual void exit(ClockMachine* clock);
    static ClockState& getInstance();
    virtual ~TimeState();
};

class AlarmState : public ClockState {
   public:
    virtual void enter(ClockMachine* clock);
    virtual void run(ClockMachine* clock);
    virtual void timerExpired(ClockMachine* clock);
    virtual void buttonShortPressed(ClockMachine* clock);
    virtual void buttonLongPressed(ClockMachine* clock);
    virtual void encoderRotated(ClockMachine* clock, rotary_encoder_pos_t position, rotary_encoder_dir_t direction = DIR_RIGHT);
    virtual void exit(ClockMachine* clock);
    static ClockState& getInstance();
    virtual ~AlarmState();

   private:
    uint8_t alarm_volume;
};

class SnoozeState : public ClockState {
   public:
    virtual void enter(ClockMachine* clock);
    virtual void run(ClockMachine* clock);
    virtual void timerExpired(ClockMachine* clock);
    virtual void buttonShortPressed(ClockMachine* clock);
    virtual void buttonLongPressed(ClockMachine* clock);
    virtual void encoderRotated(ClockMachine* clock, rotary_encoder_pos_t position, rotary_encoder_dir_t direction = DIR_RIGHT);
    virtual void exit(ClockMachine* clock);
    static ClockState& getInstance();
    virtual ~SnoozeState();

   private:
    enum {
        SNOOZE_WAITING,
        SNOOZE_FIRST_ROTATION,
        SNOOZE_LONG_PRESS,
    } snooze_leaving_step;
    rotary_encoder_dir_t first_rotation_dir;
    int64_t snooze_start_timer_s;
    uint16_t remaining_snooze_time_s;
};

class SetAlarmState : public ClockState {
   public:
    virtual void enter(ClockMachine* clock);
    virtual void run(ClockMachine* clock);
    virtual void timerExpired(ClockMachine* clock);
    virtual void buttonShortPressed(ClockMachine* clock);
    virtual void buttonLongPressed(ClockMachine* clock);
    virtual void encoderRotated(ClockMachine* clock, rotary_encoder_pos_t position, rotary_encoder_dir_t direction = DIR_RIGHT);
    virtual void exit(ClockMachine* clock);
    static ClockState& getInstance();
    virtual ~SetAlarmState();

   private:
    bool hours_hidden = false;
    bool minutes_hidden = false;
    bool setting_minutes = false;
};

#endif // _INCLUDE_CLOCK_MACHINE_STATES_HPP_