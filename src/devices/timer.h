#ifndef TIMER_H_
#define TIMER_H_

#include "IComponent.h"

#include <TimeLib.h>
#include <WString.h>
#include <binary.h>

class Timer : public IComponent
{
public:
    class AlarmHandler
    {
    public:
        virtual void on_alarm() = 0;
    };

    enum class DaysOfWeek : uint8_t
    {
        kMonday    = B00000001,
        kTuesday   = B00000010,
        kWednesday = B00000100,
        kThursday  = B00001000,
        kFriday    = B00010000,
        kSaturday  = B00100000,
        kSunday    = B01000000,
        kEveryDay  = B01111111
    };

    explicit Timer(uint32_t reading_period_ms = 500);
    void setup() override;
    void check_alarm();

    void set_alarm_str(const String& str);
    void register_alarm_handler(AlarmHandler* alarm_handler);
    void toggle_alarm();

    void   set_time_str(const String& str) const;
    String get_time_str() const;
    time_t get_time() const;

private:
    struct AlarmData
    {
        AlarmData();
        AlarmData(uint8_t h, uint8_t m, DaysOfWeek dw);
        AlarmData& operator=(const tmElements_t& time_elements);
        bool       operator==(const tmElements_t& time_elements) const;

        uint8_t    hour;
        uint8_t    minute;
        DaysOfWeek dow;
    };

    tmElements_t str_to_datetime(const String& str) const;
    AlarmData    str_to_alarm(const String& str) const;
    String       datetime_to_str(const tmElements_t& datetime) const;

    const uint32_t reading_period_ms_;
    uint32_t       last_reading_time_;
    AlarmData      alarm_;
    AlarmData      last_triggered_alarm_;
    bool           is_alarm_enabled_;
    AlarmHandler*  alarm_handler_;
};

#endif  // TIMER_H_
