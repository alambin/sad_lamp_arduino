#include "led_driver.h"

#include <Arduino.h>

#include "eeprom_map.h"

namespace
{
// TODO: there are many different options on dimming functions:
//       https://blog.moonsindustries.com/2018/12/02/what-are-dimming-curves-and-how-to-choose/)
//       Most popular (should try):
//       1. logarithmyc
//       2. DALI logarithmic
//       3. Gamma

constexpr PROGMEM uint16_t brightness_levels[] = {
    1,   2,   4,   6,   9,   12,  16,  20,  25,  30,  36,  42,  49,  56,  64,  72,  81,  90,  100, 110, 121, 132,
    144, 156, 169, 182, 196, 210, 225, 240, 256, 272, 289, 306, 324, 342, 361, 380, 400, 420, 441, 462, 484, 506,
    529, 552, 576, 600, 625, 650, 676, 702, 729, 756, 784, 812, 841, 870, 900, 930, 961, 992, 992, 992};
constexpr uint8_t num_of_levels{sizeof(brightness_levels) / sizeof(brightness_levels[0])};

// For debugging.
// constexpr PROGMEM uint16_t brightness_levels[num_of_levels] = {
//     1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,
//     21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
//     41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,
//     61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,
//     81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100,
//     101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120};


// const uint8_t PROGMEM brightness_levels_gamma8_remapped[] = {
//     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
//     0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,
//     2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   5,   5,   5,   5,   6,
//     6,   6,   6,   7,   7,   7,   7,   8,   8,   8,   9,   9,   9,   10,  10,  10,  11,  11,  11,  12,  12,  13,
//     13,  13,  14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,  20,  20,  21,  21,  22,  22,  23,  24,
//     24,  25,  25,  26,  27,  27,  28,  29,  29,  30,  31,  32,  32,  33,  34,  35,  35,  36,  37,  38,  39,  39,
//     40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  50,  51,  52,  54,  55,  56,  57,  58,  59,  60,  61,
//     62,  63,  64,  66,  67,  68,  69,  70,  72,  73,  74,  75,  77,  78,  79,  81,  82,  83,  85,  86,  87,  89,
//     90,  92,  93,  95,  96,  98,  99,  101, 102, 104, 105, 107, 109, 110, 112, 114, 115, 117, 119, 120, 122, 124,
//     126, 127, 129, 131, 133, 135, 137, 138, 140, 142, 144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167,
//     169, 171, 173, 175, 177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213, 215, 218,
//     220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255};

// constexpr PROGMEM uint8_t gammaCorrect[] = {
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02,
//     0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06,
//     0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x0A, 0x0A, 0x0A, 0x0B, 0x0B, 0x0C, 0x0C,
//     0x0C, 0x0D, 0x0D, 0x0E, 0x0E, 0x0F, 0x0F, 0x0F, 0x10, 0x10, 0x11, 0x11, 0x12, 0x12, 0x13, 0x13, 0x14, 0x14, 0x15,
//     0x16, 0x16, 0x17, 0x17, 0x18, 0x19, 0x19, 0x1A, 0x1A, 0x1B, 0x1C, 0x1C, 0x1D, 0x1E, 0x1E, 0x1F, 0x20, 0x21, 0x21,
//     0x22, 0x23, 0x24, 0x24, 0x25, 0x26, 0x27, 0x28, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2E, 0x2F, 0x30, 0x31,
//     0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x43, 0x44, 0x45,
//     0x46, 0x47, 0x48, 0x49, 0x4B, 0x4C, 0x4D, 0x4E, 0x50, 0x51, 0x52, 0x53, 0x55, 0x56, 0x57, 0x59, 0x5A, 0x5B, 0x5D,
//     0x5E, 0x5F, 0x61, 0x62, 0x63, 0x65, 0x66, 0x68, 0x69, 0x6B, 0x6C, 0x6E, 0x6F, 0x71, 0x72, 0x74, 0x75, 0x77, 0x79,
//     0x7A, 0x7C, 0x7D, 0x7F, 0x81, 0x82, 0x84, 0x86, 0x87, 0x89, 0x8B, 0x8D, 0x8E, 0x90, 0x92, 0x94, 0x96, 0x97, 0x99,
//     0x9B, 0x9D, 0x9F, 0xA1, 0xA3, 0xA5, 0xA6, 0xA8, 0xAA, 0xAC, 0xAE, 0xB0, 0xB2, 0xB4, 0xB6, 0xB8, 0xBA, 0xBD, 0xBF,
//     0xC1, 0xC3, 0xC5, 0xC7, 0xC9, 0xCC, 0xCE, 0xD0, 0xD2, 0xD4, 0xD7, 0xD9, 0xDB, 0xDD, 0xE0, 0xE2, 0xE4, 0xE7, 0xE9,
//     0xEB, 0xEE, 0xF0, 0xF3, 0xF5, 0xF8, 0xFA, 0xFD, 0xFF};

uint8_t
invert_level(uint8_t level)
{
    return 255 - level;
}

}  // namespace

LedDriver::LedDriver(uint8_t pin, Pwm::PWMSpeed pwm_speed, uint32_t updating_period_ms)
  : pwm_{pin, pwm_speed, false}
  , initial_updating_period_ms_{updating_period_ms}
  , adjusted_updating_period_ms_(updating_period_ms)
  , last_updating_time_{0}
  , is_sunrise_in_progress_{false}
  , sunrise_start_time_{0}
  , sunrise_duration_sec_{0}
  , current_brightness_{0}
{
}

void
LedDriver::setup()
{
    pwm_.setup();
    uint16_t duration_min{(uint16_t)eeprom_read_word(&sunraise_duration_minutes_address)};
    set_sunrise_duration(duration_min);
    set_brightness(0);

    Serial.print(F("Read from EEPROM: Sunrise duration "));
    Serial.print(duration_min);
    Serial.println(F(" minutes"));
}

void
LedDriver::run_sunrise()
{
    if (!is_sunrise_in_progress_) {
        return;
    }

    // Do not execute too often
    auto now = millis();
    if ((now - last_updating_time_) < adjusted_updating_period_ms_) {
        return;
    }
    last_updating_time_ = now;

    uint32_t delta_time_ms{(now - sunrise_start_time_)};
    if (delta_time_ms >= (sunrise_duration_sec_ * 1000)) {
        // Sunrise is finished
        set_brightness(1023);  // Keep lamp turned on
        return;
    }

    pwm_.set_duty(map_sunrise_time_to_level(delta_time_ms));
}

void
LedDriver::set_sunrise_duration_str(const String& str)
{
    Serial.print(F("Received command 'Set Sunrise duration' "));
    Serial.println(str);

    uint16_t duration_min{(uint16_t)str.substring(0, 4).toInt()};
    eeprom_write_word(&sunraise_duration_minutes_address, duration_min);
    set_sunrise_duration(duration_min);

    Serial.print(F("Stored to EEPROM Sunrise duration "));
    Serial.print(duration_min);
    Serial.println(F(" minutes"));
}

String
LedDriver::get_sunrise_duration_str() const
{
    // MMMM
    char str[5];
    // A terminating null character is automatically appended by snprintf
    snprintf_P(str, 5, PSTR("%04d"), sunrise_duration_sec_ / 60);
    return String(str);
}

void
LedDriver::start_sunrise()
{
    is_sunrise_in_progress_ = true;
    sunrise_start_time_     = millis();
}

void
LedDriver::stop_sunrise()
{
    is_sunrise_in_progress_ = false;
}

void
LedDriver::set_brightness(uint16_t level)
{
    stop_sunrise();  // Manual control of brightness cancells sunrise
    pwm_.set_duty(map_manual_control_to_level(level));
}

void
LedDriver::set_brightness_str(const String& str)
{
    Serial.print(F("Received command 'Set brightness' "));
    Serial.println(str);

    uint16_t brightness{(uint16_t)str.substring(0, 4).toInt()};
    set_brightness(brightness);
}

String
LedDriver::get_brightness_str() const
{
    // BBBB
    char str[5];
    // A terminating null character is automatically appended by snprintf
    snprintf_P(str, 5, PSTR("%04d"), current_brightness_);
    return String(str);
}

void
LedDriver::set_sunrise_duration(uint16_t duration_m)
{
    sunrise_duration_sec_ = (uint32_t)duration_m * 60;

    // Adjust updating period
    adjusted_updating_period_ms_ = min(initial_updating_period_ms_, (sunrise_duration_sec_ * 1000) / num_of_levels);
}

uint8_t
LedDriver::map_sunrise_time_to_level(uint32_t delta_time_ms)
{
    // Avoid overflow
    uint8_t index;
    if (delta_time_ms <= 0xFFFFFF) {
        // This will not cause overflow
        index = (delta_time_ms * num_of_levels) / (sunrise_duration_sec_ * 1000);
    }
    else {
        // Max delta is 24 hours = 24*60*60*1000. So, this code will not cause overflow
        index = ((delta_time_ms / 1000) * num_of_levels) / sunrise_duration_sec_;
    }

    uint16_t brightness_level{pgm_read_word(&brightness_levels[index])};
    uint8_t  mapped_level{map(brightness_level, 1, 992, 0, 255)};
    current_brightness_ = mapped_level << 2;

    // Return inverted PWM duty cycle, because current 100% duty cycle makes 0 ohm on DIM input of LED driver, which
    // corresponds to 0% brightness
    return invert_level(mapped_level);
}

uint8_t
LedDriver::map_manual_control_to_level(uint16_t manual_level)
{
    current_brightness_ = manual_level;

    // manual_level is read from potentiometer. Usually dependency of potentiometer's resistance from rotation angle is
    // not lineral, so this function's goal is to provide mapping between real readings from potentiometer and LED
    // brightness level
    // In theory eyes perception curve lays above y=x, and resistance-angle curve lays below y=x. So, they should
    // compensate each other.
    // In practice - I don't see any difference between mapping functions.
    // Probably we don't need mapping here. User is setting brightness manually, so he will choose brightness as he
    // wants by changing angle of potentiometer.

    // Return inverted PWM duty cycle, because current 100% duty cycle makes 0 ohm on DIM input of LED driver, which
    // corresponds to 0% brightness
    return invert_level(manual_level >> 2);
}
