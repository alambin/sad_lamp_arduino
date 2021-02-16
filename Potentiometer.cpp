#include "Potentiometer.h"

#include <Arduino.h>

Potentiometer::Potentiometer(uint8_t pin, uint32_t sampling_ms)
  : pin_{pin}
  , sampling_ms_{sampling_ms}
  , current_sampe_index_{0}
  , running_average_value_{0}
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
    static float filVal = 0;
    running_average_value_ += (new_value - running_average_value_) * k;
    return running_average_value_;
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

    Serial.print("NotFiltered:");
    Serial.print(new_value);
    // Serial.print(", median10Gyver:");
    // Serial.print(median_10gyver_val);
    // Serial.print(", median3Gyver:");
    // Serial.print(median_3gyver_val);
    Serial.print(", median3GyverAvg:");
    Serial.print((uint16_t)(filterRunningAverage(median_3gyver_val)));
    Serial.print(", median3GyverAvgAdaptive:");
    Serial.print((uint16_t)(filterRunningAverageAdaptive(median_3gyver_val)));
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
    // static uint32_t median_om9_avg_time = 0;

    // static bool reset_made = false;
    // if ((runc_count >= 100) || reset_made) {
    //     if (!reset_made) {
    //         runc_count = 1;
    //         reset_made = true;
    //     }

    //     median_10gyver_avg_time += median_10gyver_time;
    //     median_3gyver_avg_time += median_3gyver_time;
    //     median_om9_avg_time += median_om9_time;
    // }

    // Serial.print("median_gyver_avg_time:");
    // Serial.print(median_10gyver_avg_time / runc_count);
    // Serial.print(", median_gyver_opt_avg_time:");
    // Serial.print(median_3gyver_avg_time / runc_count);
    // Serial.print(", median_om9_avg_time:");
    // Serial.print(median_om9_avg_time / runc_count);
    // Serial.println();
}