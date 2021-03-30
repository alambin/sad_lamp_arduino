#include "potentiometer.h"

#include <Arduino.h>

Potentiometer::Potentiometer(uint8_t pin, uint32_t sampling_ms)
  : pin_{pin}
  , sampling_ms_{sampling_ms}
  , current_sampe_index_{0}
  , running_average_value_{0}
  , running_average_int_value_{0}
  , current_value_{0}
  , last_sampling_time_{0}
{
    for (auto& sample : samples) {
        sample = 0;
    }
}

void
Potentiometer::setup()
{
    pinMode(pin_, INPUT);
}

void
Potentiometer::loop()
{
    auto now = millis();
    if ((now - last_sampling_time_) < sampling_ms_) {
        return;
    }
    last_sampling_time_ = now;

    current_value_ = filter(analogRead(pin_));
}

uint16_t
Potentiometer::read()
{
    return current_value_;
}

uint16_t
Potentiometer::filter(uint16_t new_value)
{
    // PrintprintDebug(new_value);

    return (uint16_t)filterRunningAverageAdaptive(filterMedian3(new_value));
}

uint16_t
Potentiometer::filterMedianN(uint16_t new_value)
{
    samples[current_sampe_index_] = new_value;
    if ((current_sampe_index_ < samples_size_ - 1) &&
        (samples[current_sampe_index_] > samples[current_sampe_index_ + 1])) {
        for (int i = current_sampe_index_; i < samples_size_ - 1; ++i) {
            if (samples[i] > samples[i + 1]) {
                // Swap. Swapping via XOR is slowlier
                uint16_t tmp   = samples[i];
                samples[i]     = samples[i + 1];
                samples[i + 1] = tmp;
            }
        }
    }
    else {
        if ((current_sampe_index_ > 0) && (samples[current_sampe_index_ - 1] > samples[current_sampe_index_])) {
            for (int i = current_sampe_index_; i > 0; --i) {
                if (samples[i] < samples[i - 1]) {
                    // Swap. Swapping via XOR is slowlier
                    uint16_t tmp   = samples[i];
                    samples[i]     = samples[i - 1];
                    samples[i - 1] = tmp;
                }
            }
        }
    }
    if (++current_sampe_index_ >= samples_size_) {
        current_sampe_index_ = 0;
    }
    return samples[samples_size_ / 2];
}

// Fast version of median filter for 3 samples. Can give good results in combination with running average filter
uint16_t
Potentiometer::filterMedian3(uint16_t new_value)
{
    samples[current_sampe_index_] = new_value;
    if (++current_sampe_index_ >= 3) {
        current_sampe_index_ = 0;
    }

    uint16_t& a = samples[0];
    uint16_t& b = samples[1];
    uint16_t& c = samples[2];
    if ((a <= b) && (a <= c)) {
        return (b <= c) ? b : c;
    }
    else {
        if ((b <= a) && (b <= c)) {
            return (a <= c) ? a : c;
        }
        else {
            return (a <= b) ? a : b;
        }
    }
}

float
Potentiometer::filterRunningAverage(float new_value, float k = 0.5f)
{
    running_average_value_ += (new_value - running_average_value_) * k;
    return running_average_value_;
}

// Int version is slightly slowlier, but looks the same precise.
// Performance: 8-10 times faster
// Memory: 5674/462 -> 5630/464. Surprisingly it requires 2 more bytes or RAM. But 44 less bytes of flash
int16_t
Potentiometer::filterRunningAverageInt(int16_t new_value)
{
    running_average_int_value_ += (new_value - running_average_int_value_) / 2;
    return running_average_int_value_;
}

// https://alexgyver.ru/lessons/filters/
// This filter better adapts to fast changes of value
float
Potentiometer::filterRunningAverageAdaptive(float new_value, float k_slow, float k_fast, float threshold)
{
    float k;
    // Speed of filter depends on absolute value of difference
    if (abs(new_value - running_average_value_) > threshold)
        k = k_fast;
    else
        k = k_slow;

    running_average_value_ += (new_value - running_average_value_) * k;
    return running_average_value_;
}

// Int version the same fast as float version. It looks like litle bit less precise. Ex. it is always 2-3 higher or
// lower than original signal or float filter value. Idk why. Sometimes it doesn't react on signal change.
// Ex. signal = 8, int_filter = 6, then signal = 4, int_filter = 6
// This is because of division on 10. Small changes will be invisible for int version, but anyway it is still quite
// precise
// Performance: 2.5-2.8 times faster
// Memory: 5746/470 -> 5760/472. Surprisingly it requires 2 more bytes or RAM. AND 16 more bytes of flash!
int16_t
Potentiometer::filterRunningAverageAdaptiveInt(int16_t new_value,
                                               int8_t  k_slow    = 3,
                                               int8_t  k_fast    = 9,
                                               int8_t  threshold = 10)
{
    int8_t k;
    // Speed of filter depends on absolute value of difference
    if (abs(new_value - running_average_int_value_) > threshold)
        k = k_fast;
    else
        k = k_slow;

    running_average_int_value_ += ((new_value - running_average_int_value_) * k) / 10;
    return running_average_int_value_;
}

void
Potentiometer::PrintprintDebug(uint16_t new_value)
{
    auto     start_time          = micros();
    uint16_t median_10gyver_val  = filterMedianN(new_value);
    auto     median_10gyver_time = micros() - start_time;
    start_time                   = micros();
    uint16_t median_3gyver_val   = filterMedian3(new_value);
    auto     median_3gyver_time  = micros() - start_time;
    start_time                   = micros();

    uint16_t avg_val  = (uint16_t)(filterRunningAverage(median_3gyver_val));
    auto     avg_time = micros() - start_time;
    start_time        = micros();
    // uint16_t avg_int_val  = filterRunningAverageInt(median_3gyver_val);
    // auto     avg_int_time = micros() - start_time;
    // start_time            = micros();

    uint16_t avg_adp_val  = (uint16_t)(filterRunningAverageAdaptive(median_3gyver_val));
    auto     avg_adp_time = micros() - start_time;
    start_time            = micros();
    // uint16_t avg_adp_int_val  = filterRunningAverageAdaptiveInt(median_3gyver_val);
    // auto     avg_adp_int_time = micros() - start_time;
    // start_time                = micros();

    Serial.print("NotFiltered:");
    Serial.print(new_value);
    // Serial.print(", median10Gyver:");
    // Serial.print(median_10gyver_val);
    // Serial.print(", median3Gyver:");
    // Serial.print(median_3gyver_val);
    Serial.print(", median3GyverAvg:");
    Serial.print(avg_val);
    // Serial.print(", median3GyverAvgInt:");
    // Serial.print(avg_int_val);
    Serial.print(", median3GyverAvgAdaptive:");
    Serial.print(avg_adp_val);
    // Serial.print(", median3GyverAvgAdaptiveInt:");
    // Serial.print(avg_adp_int_val);
    // Serial.print(", medianNth:");
    // Serial.print(median_nth_val);
    Serial.println();

    // Print performance
    // Serial.print("median_10gyver_time:");
    // Serial.print(median_10gyver_time);
    // Serial.println();

    // Print avg performance
    // static uint32_t runc_count = 0;
    // ++runc_count;
    // static uint32_t median_10gyver_avg_time     = 0;
    // static uint32_t median_3gyver_avg_time = 0;
    // static uint32_t avg_avg_time     = 0;
    // static uint32_t avg_int_avg_time = 0;
    // static uint32_t avg_adp_avg_time     = 0;
    // static uint32_t avg_adp_int_avg_time = 0;

    // static bool reset_made = false;
    // if ((runc_count >= 100) || reset_made) {
    //     if (!reset_made) {
    //         runc_count = 1;
    //         reset_made = true;
    //     }

    //     median_10gyver_avg_time += median_10gyver_time;
    //     median_3gyver_avg_time += median_3gyver_time;
    //     avg_avg_time += avg_time;
    //     avg_int_avg_time += avg_int_time;
    //     avg_adp_avg_time += avg_adp_time;
    //     avg_adp_int_avg_time += avg_adp_int_time;
    // }

    // Serial.print("median_gyver_avg_time:");
    // Serial.print(median_10gyver_avg_time / runc_count);
    // Serial.print(", median_gyver_opt_avg_time:");
    // Serial.print(median_3gyver_avg_time / runc_count);
    // Serial.print("avg_avg_time:");
    // Serial.print(avg_avg_time / runc_count);
    // Serial.print(", avg_int_avg_time:");
    // Serial.print(avg_int_avg_time / runc_count);
    // Serial.print("avg_adp_avg_time:");
    // Serial.print(avg_adp_avg_time / runc_count);
    // Serial.print(", avg_adp_int_avg_time:");
    // Serial.print(avg_adp_int_avg_time / runc_count);
    // Serial.println();
}
