#include "timer.h"

#include <limits.h>

#include <Arduino.h>
#include <DS1307RTC.h>

#include "eeprom_map.h"

namespace
{
Timer::DaysOfWeek
TimelibWDayToDOW(uint8_t c)
{
    switch (c) {
    case 1:
        return Timer::DaysOfWeek::kSunday;
    case 2:
        return Timer::DaysOfWeek::kMonday;
    case 3:
        return Timer::DaysOfWeek::kTuesday;
    case 4:
        return Timer::DaysOfWeek::kWednesday;
    case 5:
        return Timer::DaysOfWeek::kThursday;
    case 6:
        return Timer::DaysOfWeek::kFriday;
    case 7:
        return Timer::DaysOfWeek::kSaturday;
    default:
        return Timer::DaysOfWeek::kEveryDay;
    }
}
}  // namespace

Timer::Timer(uint32_t reading_period_ms = 500)
  : reading_period_ms_{reading_period_ms}
  , last_triggered_alarm_{0xFF, 0xFF, DaysOfWeek::kEveryDay}
  , is_alarm_enabled_{false}
  , alarm_handler_{nullptr}
{
}

void
Timer::Setup()
{
    alarm_.hour       = eeprom_read_byte(&alarm_hours_address);
    alarm_.minute     = eeprom_read_byte(&alarm_minutes_address);
    alarm_.dow        = static_cast<Timer::DaysOfWeek>(eeprom_read_byte(&alarm_dow_address));
    is_alarm_enabled_ = (eeprom_read_byte(&is_alarm_on_address) == 1);

    Serial.print(F("Read from EEPROM: alarm time "));
    Serial.print((int)alarm_.hour);
    Serial.print(F(":"));
    Serial.print((int)alarm_.minute);
    Serial.print(F(" DoW= 0x"));
    Serial.print((int)alarm_.dow, HEX);
    Serial.print(F(". Alarm is "));
    Serial.println(is_alarm_enabled_ ? F("enabled") : F("disabled"));
}

// We are not using hardware alarm. Reason: there are only 2 alarms. They can be configured to trigger either on
// specified day of week, either on specified day of month. But in case of SAD Lamp we want alarm to trigger every day.
// The only option to do it is to check current hour and minute in Arduino's main loop.
void
Timer::CheckAlarm()
{
    if ((!is_alarm_enabled_) || (alarm_handler_ == nullptr)) {
        return;
    }

    // Do not read from RTC on every iteration of loop(). Reading 2 times per second is quite safe.
    static uint32_t last_reading_time{0};
    auto            now = millis();
    if ((now - last_reading_time) < reading_period_ms_) {
        return;
    }
    last_reading_time = now;

    tmElements_t datetime;
    if (!RTC.read(datetime)) {
        return;
    }

    bool does_dow_match{(uint8_t)alarm_.dow & (uint8_t)TimelibWDayToDOW(datetime.Wday)};
    if (does_dow_match && (datetime.Hour == alarm_.hour) && (datetime.Minute == alarm_.minute) &&
        (datetime.Second == 0)) {
        // Trigger alarm only once
        if (!(last_triggered_alarm_ == datetime)) {
            last_triggered_alarm_ = datetime;
            alarm_handler_->OnAlarm();
        }
    }
}

void
Timer::RegisterAlarmHandler(AlarmHandler* alarm_handler)
{
    alarm_handler_ = alarm_handler;
}

String
Timer::GetTimeStr() const
{
    tmElements_t datetime;
    String       result;
    if (RTC.read(datetime)) {
        result = DatetimeToStr(datetime);
    }
    return result;
}

time_t
Timer::GetTime() const
{
    return RTC.get();
}

void
Timer::SetTimeStr(const String& str) const
{
    Serial.print(F("Received command 'Set time' "));
    Serial.println(str);

    auto datetime{StrToDatetime(str)};
    RTC.write(datetime);
}

void
Timer::SetAlarmStr(const String& str)
{
    Serial.print(F("Received command 'Set alarm' "));
    Serial.println(str);

    alarm_ = StrToAlarm(str);

    // Store new alarm value in EEPROM
    eeprom_write_byte(&alarm_hours_address, alarm_.hour);
    eeprom_write_byte(&alarm_minutes_address, alarm_.minute);
    eeprom_write_byte(&alarm_dow_address, (uint8_t)(alarm_.dow));

    Serial.print(F("Stored to EEPROM alarm at "));
    Serial.print((int)alarm_.hour);
    Serial.print(F(":"));
    Serial.print((int)alarm_.minute);
    Serial.print(F(" DoW= 0x"));
    Serial.println((int)alarm_.dow, HEX);
}

String
Timer::GetAlarmStr() const
{
    // E HH:MM WW
    char str[11];

    str[0] = (is_alarm_enabled_) ? 'E' : 'D';
    str[1] = ' ';

    // A terminating null character is automatically appended by snprintf
    snprintf_P(str + 2, 9, PSTR("%02d:%02d %02x"), alarm_.hour, alarm_.minute, (uint8_t)alarm_.dow);
    return String(str);
}

bool
Timer::EnableAlarmStr(const String& str)
{
    Serial.print(F("Received command 'Enable alarm' "));
    Serial.println(str);

    if (str[0] == 'E') {
        if (!is_alarm_enabled_) {
            ToggleAlarm();
        }
        return true;
    }
    else if (str[0] == 'D') {
        if (is_alarm_enabled_) {
            ToggleAlarm();
        }
        return true;
    }
    return false;
}

void
Timer::ToggleAlarm()
{
    if (is_alarm_enabled_) {
        is_alarm_enabled_ = false;
        eeprom_write_byte(&is_alarm_on_address, 0);
    }
    else {
        is_alarm_enabled_ = true;
        eeprom_write_byte(&is_alarm_on_address, 1);
    }

    Serial.print(F("Alarm is "));
    Serial.println(is_alarm_enabled_ ? F("enabled") : F("disabled"));
}

Timer::AlarmData::AlarmData()
  : AlarmData(0, 0, Timer::DaysOfWeek::kEveryDay)
{
}

Timer::AlarmData::AlarmData(uint8_t h, uint8_t m, DaysOfWeek dw)
  : hour{h}
  , minute{m}
  , dow{dw}
{
}

Timer::AlarmData&
Timer::AlarmData::operator=(const tmElements_t& time_elements)
{
    hour   = time_elements.Hour;
    minute = time_elements.Minute;
    // Do NOT take DoW into account
    return *this;
}

bool
Timer::AlarmData::operator==(const tmElements_t& time_elements) const
{
    // Do NOT take DoW into account
    return ((hour == time_elements.Hour) && (minute == time_elements.Minute));
}

tmElements_t
Timer::StrToDatetime(const String& str) const
{
    tmElements_t tm;
    // HH:MM:SS DD/MM/YYYY
    tm.Hour   = str.substring(0, 2).toInt();
    tm.Minute = str.substring(3, 5).toInt();
    tm.Second = str.substring(6, 8).toInt();
    tm.Day    = str.substring(9, 11).toInt();
    tm.Month  = str.substring(12, 14).toInt();
    tm.Year   = CalendarYrToTm(str.substring(15, 19).toInt());

    // We have to set week day manually, because DS1307RTC library doesn't do it.
    // We are doing this calculation by building time_t (seconds since 19700) from input data and by converting it
    // back to tmElements_t by breakTime() function. This function calculates week day.
    tmElements_t datetime;
    breakTime(makeTime(tm), datetime);

    return datetime;
}

Timer::AlarmData
Timer::StrToAlarm(const String& str) const
{
    // HH:MM
    uint8_t h = str.substring(0, 2).toInt();
    uint8_t m = str.substring(3, 5).toInt();

    DaysOfWeek dow = DaysOfWeek::kEveryDay;
    if (str.length() >= 7) {
        auto res = strtoul(str.substring(6, 8).c_str(), 0, 16);
        if (!((res == 0) || (res == ULONG_MAX))) {
            dow = static_cast<Timer::DaysOfWeek>(res & 0x7F);
        }
    }

    return Timer::AlarmData{h, m, dow};
}

String
Timer::DatetimeToStr(const tmElements_t& datetime) const
{
    // H:MM:SS DD/MM/YYYY
    char str[20];
    // A terminating null character is automatically appended by snprintf
    snprintf_P(str,
               20,
               PSTR("%02d:%02d:%02d %02d/%02d/%04d"),
               datetime.Hour,
               datetime.Minute,
               datetime.Second,
               datetime.Day,
               datetime.Month,
               tmYearToCalendar(datetime.Year));
    return String(str);
}
