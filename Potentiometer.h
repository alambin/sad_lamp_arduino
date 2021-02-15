#ifndef POTENTIOMETER_H_
#define POTENTIOMETER_H_

#include "IComponent.h"

#include <stdint.h>

class Potentiometer : public IComponent
{
public:
    Potentiometer(uint8_t pin, uint32_t sampling_ms);
    void     setup() override;
    void     loop();
    uint16_t read();

private:
    uint16_t filter(uint16_t new_value);

    // Different filters. You can experiment with settigns of each filter and with their combinations
    uint16_t filterMedianN(uint16_t new_value);
    uint16_t filterMedian3(uint16_t new_value);
    float    filterRunningAverage(float new_value, float k = 0.5f);
    float    filterRunningAverageAdaptive(float new_value,
                                          float k_slow    = 0.3f,
                                          float k_fast    = 0.9f,
                                          float threshold = 10.0f);

    const uint8_t  pin_;
    const uint32_t sampling_ms_;

    static constexpr uint8_t samples_size_ = 10;
    uint16_t                 samples[samples_size_];
    uint8_t                  current_sampe_index_;
    float                    running_average_value_;
    uint16_t                 current_value_;
    uint32_t                 last_sampling_time_;

    // This is for debugging and confuguring filters
    void PrintprintDebug(uint16_t new_value);
};

#endif  // POTENTIOMETER_H_
