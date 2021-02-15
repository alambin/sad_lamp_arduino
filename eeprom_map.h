#ifndef EEPROM_MAP_H_
#define EEPROM_MAP_H_

#include <EEPROM.h>

// EEMEM macro will automatically give addresses in EEPROM memory.
// BUT it will give lower addresses to variables lower in this file

extern uint8_t EEMEM  fan_pwm_steps_number_address;  // 6
extern uint16_t EEMEM fan_pwm_frequency_address;     // 4-5

extern uint8_t EEMEM sunraise_duration_minutes_address;  // 3

extern uint8_t EEMEM is_alarm_on_address;    // 2
extern uint8_t EEMEM alarm_hours_address;    // 1
extern uint8_t EEMEM alarm_minutes_address;  // 0

#endif  // EEPROM_MAP_H_
