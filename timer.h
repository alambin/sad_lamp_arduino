#ifndef TIMER_H_
#define TIMER_H_

#include "IComponent.h"

#include <Time.h>
#include <WString.h>

class Timer : public IComponent
{
public:
    using AlarmCallbackType = void (*)();
    explicit Timer(uint32_t reading_period_ms = 500);
    void setup() override;
    void loop();

    void set_alarm_str(const String& str);
    void set_alarm_callback(AlarmCallbackType callback);
    void toggle_alarm();

    void   set_time_str(const String& str) const;
    String get_time_str() const;
    time_t get_time() const;

private:
    struct AlarmData
    {
        uint8_t hour;
        uint8_t minute;
    };

    class AlarmDataExtended
    {
    public:
        AlarmDataExtended();
        explicit AlarmDataExtended(const tmElements_t& time_elements);
        AlarmDataExtended& operator=(const tmElements_t& time_elements);
        bool               operator==(const tmElements_t& time_elements) const;

    private:
        AlarmData alarm_data;
        uint8_t   day;
        uint8_t   month;
        uint8_t   year;
    };

    tmElements_t str_to_datetime(const String& str) const;
    AlarmData    str_to_alarm(const String& str) const;
    String       datetime_to_str(const tmElements_t& datetime) const;

    const uint32_t    reading_period_ms_;
    uint32_t          last_reading_time_;
    AlarmData         alarm_;
    AlarmDataExtended last_triggered_alarm_;
    bool              is_alarm_enabled_;
    AlarmCallbackType callback_;
};

#endif  // TIMER_H_
