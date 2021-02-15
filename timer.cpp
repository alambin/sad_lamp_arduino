#include "timer.h"

#include <Arduino.h>
#include <DS1307RTC.h>
// #include <stdio.h>
// #include "HardwareSerial.h"

#include "eeprom_map.h"

Timer::Timer(uint32_t reading_period_ms = 500)
  : reading_period_ms_{reading_period_ms}
  , last_reading_time_{0}
  , alarm_{0, 0}
  , is_alarm_enabled_{false}
  , callback_{nullptr}
{
}

void
Timer::setup()
{
    alarm_.hour       = eeprom_read_byte(&alarm_hours_address);
    alarm_.minute     = eeprom_read_byte(&alarm_minutes_address);
    is_alarm_enabled_ = (eeprom_read_byte(&is_alarm_on_address) == 1);

    Serial.print(F("Read from EEPROM alarm time "));
    Serial.print((int)alarm_.hour);
    Serial.print(F(":"));
    Serial.print((int)alarm_.minute);
    Serial.print(F(". Alarm is "));
    Serial.println(is_alarm_enabled_ ? F("enabled") : F("disabled"));
}

// We are not using hardware alarm. Reason: there are only 2 alarms. They can be configured to trigger either on
// specified day of week, either on specified day of month. But in case of SAD Lamp we want alarm to trigger every day.
// The only option to do it is to check current hour and minute in Arduino's main loop.
void
Timer::loop()
{
    if ((!is_alarm_enabled_) || (callback_ == nullptr)) {
        return;
    }

    // Do not read from RTC on every iteration of loop(). Reading 2 times per second is quite safe.
    auto now = millis();
    if ((now - last_reading_time_) < reading_period_ms_) {
        return;
    }
    last_reading_time_ = now;

    tmElements_t datetime;
    if (RTC.read(datetime) &&
        ((datetime.Hour == alarm_.hour) && (datetime.Minute == alarm_.minute) && (datetime.Second == 0))) {
        // Trigger alarm only once
        if (!(last_triggered_alarm_ == datetime)) {
            last_triggered_alarm_ = datetime;
            callback_();
        }
    }
}

void
Timer::set_alarm_callback(Timer::AlarmCallbackType callback)
{
    callback_ = callback;
}

String
Timer::get_time_str() const
{
    tmElements_t datetime;
    String       result;
    if (RTC.read(datetime)) {
        result = datetime_to_str(datetime);
    }
    return result;
}

time_t
Timer::get_time() const
{
    return RTC.get();
}

void
Timer::set_time_str(const String& str) const
{
    Serial.print(F("Received command 'Set time' "));
    Serial.println(str);

    auto datetime{str_to_datetime(str)};
    RTC.write(datetime);
}

void
Timer::set_alarm_str(const String& str)
{
    Serial.print(F("Received command 'Set alarm' "));
    Serial.println(str);

    alarm_ = str_to_alarm(str);

    // Store new alarm value in EEPROM
    eeprom_write_byte(&alarm_hours_address, alarm_.hour);
    eeprom_write_byte(&alarm_minutes_address, alarm_.minute);

    Serial.print(F("Stored to EEPROM alarm at "));
    Serial.print((int)alarm_.hour);
    Serial.print(F(":"));
    Serial.println((int)alarm_.minute);
}

void
Timer::toggle_alarm()
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

Timer::AlarmDataExtended::AlarmDataExtended()
  : alarm_data{0, 0}
  , day{0}
  , month{0}
  , year{0}
{
}

Timer::AlarmDataExtended::AlarmDataExtended(const tmElements_t& time_elements)
  : alarm_data{time_elements.Hour, time_elements.Minute}
  , day{time_elements.Day}
  , month{time_elements.Month}
  , year{time_elements.Year}
{
}

Timer::AlarmDataExtended&
Timer::AlarmDataExtended::operator=(const tmElements_t& time_elements)
{
    AlarmDataExtended tmp(time_elements);
    *this = tmp;
    return *this;
}

bool
Timer::AlarmDataExtended::operator==(const tmElements_t& time_elements) const
{
    return ((alarm_data.hour == time_elements.Hour) && (alarm_data.minute == time_elements.Minute) &&
            (day == time_elements.Day) && (month == time_elements.Month) && (year == time_elements.Year));
}

tmElements_t
Timer::str_to_datetime(const String& str) const
{
    tmElements_t datetime;
    // HH:MM:SS DD/MM/YYYY
    datetime.Hour   = str.substring(0, 2).toInt();
    datetime.Minute = str.substring(3, 5).toInt();
    datetime.Second = str.substring(6, 8).toInt();
    datetime.Day    = str.substring(9, 11).toInt();
    datetime.Month  = str.substring(12, 14).toInt();
    datetime.Year   = CalendarYrToTm(str.substring(15, 19).toInt());
    return datetime;
}

Timer::AlarmData
Timer::str_to_alarm(const String& str) const
{
    // HH:MM
    uint8_t h = str.substring(0, 2).toInt();
    uint8_t m = str.substring(3, 5).toInt();
    return {h, m};
}

String
Timer::datetime_to_str(const tmElements_t& datetime) const
{
    char str[20];
    snprintf(str,
             20,
             "%02d:%02d:%02d %02d/%02d/%04d",
             datetime.Hour,
             datetime.Minute,
             datetime.Second,
             datetime.Day,
             datetime.Month,
             tmYearToCalendar(datetime.Year));
    return String(str);
}
