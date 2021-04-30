#ifndef POTENTIOMETER_H_
#define POTENTIOMETER_H_

#include "IComponent.h"

#include <stdint.h>

class Potentiometer : public IComponent
{
public:
    Potentiometer(uint8_t pin, uint32_t sampling_ms);
    void     Setup() override;
    void     Loop();
    uint16_t Read() const;

private:
    uint16_t Filter(uint16_t new_value);

    // Different filters. You can experiment with settigns of each filter and with their combinations
    uint16_t FilterMedianN(uint16_t new_value);
    uint16_t FilterMedian3(uint16_t new_value);
    float    FilterRunningAverage(float new_value, float k = 0.5f);
    float    FilterRunningAverageAdaptive(float new_value,
                                          float k_slow    = 0.3f,
                                          float k_fast    = 0.9f,
                                          float threshold = 10.0f);
    // Int versions of RunningAverage. They are faster, but requires almost the same memory. More details in .cpp
    int16_t FilterRunningAverageInt(int16_t new_value);
    int16_t FilterRunningAverageAdaptiveInt(int16_t new_value,
                                            int8_t  k_slow    = 3,
                                            int8_t  k_fast    = 9,
                                            int8_t  threshold = 10);

    const uint8_t  pin_;
    const uint32_t sampling_ms_;

    static constexpr uint8_t samples_size_ = 10;
    uint16_t                 samples[samples_size_];
    uint8_t                  current_sampe_index_;
    float                    running_average_value_;
    int16_t                  running_average_int_value_;
    uint16_t                 current_value_;

    // This is for debugging and confuguring filters
    // void PrintprintDebug(uint16_t new_value);
};

#endif  // POTENTIOMETER_H_
