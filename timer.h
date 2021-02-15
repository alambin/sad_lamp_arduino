#ifndef TIMER_H_
#define TIMER_H_

#include "IComponent.h"

#include <Time.h>
#include "WString.h"

class Timer
  : public IComponent
{
public:
    using AlarmCallbackType = void (*)();
    Timer();
    void setup() override;
    void loop();

    void set_alarm_str(const String& str);
    void set_alarm_callback(AlarmCallbackType callback);
    void toggle_alarm();

    void set_time_str(const String& str) const;
    String get_time_str() const;
    time_t get_time() const;

private:
    struct AlarmData
    {
        uint8_t hour;
        uint8_t minute;
    };

    tmElements_t str_to_datetime(const String& str) const;
    AlarmData    str_to_alarm(const String& str) const;
    String       datetime_to_str(const tmElements_t& datetime) const;

    AlarmData         alarm_;
    bool              is_alarm_enabled_;
    AlarmCallbackType callback_;
};

#endif  // TIMER_H_
