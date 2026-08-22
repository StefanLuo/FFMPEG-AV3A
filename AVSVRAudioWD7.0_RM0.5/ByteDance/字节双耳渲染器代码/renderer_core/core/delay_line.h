/* Copyright 2021 Beijing Zitiao Network Technology Co.,
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#pragma once
#include <limits>
#include "core/ring_buffer.h"
#include <atomic>
#include <cmath>

namespace avs3renderer {

template <typename DataType, typename TimeType = float>
class DelayLine {
public:
    explicit DelayLine(TimeType max_delay_in_samples, TimeType initial_delay_in_samples = 0)
        : ring_buffer_(static_cast<size_t>(std::ceil(max_delay_in_samples)), GetDefaultElementValue()),
          delay_target_(initial_delay_in_samples),
          current_delay_(initial_delay_in_samples),
          delay_per_sample_increment_(0.f),
          use_default_speed_(true) {
        //  Shift write index one step ahead of read index
        ring_buffer_.OffsetWriteIndex(1);
    }

    DelayLine(const DelayLine<DataType>& delay_line)
        : ring_buffer_(delay_line.ring_buffer_.max_size(), GetDefaultElementValue()),
          delay_target_(delay_line.delay_target_.load()),
          current_delay_(delay_line.current_delay_.load()),
          delay_per_sample_increment_(delay_line.delay_per_sample_increment_.load()),
          use_default_speed_(delay_line.use_default_speed_.load()) {
    }

    void SetDelayInSamples(TimeType delay_target) {
        delay_target_ = delay_target;
        if (use_default_speed_.load())
            delay_per_sample_increment_ = (delay_target_ - current_delay_) / kDelayInterpolationSteps;
        else
            delay_per_sample_increment_ = delay_target_ >= current_delay_ ? std::abs(delay_per_sample_increment_)
                                                                          : -std::abs(delay_per_sample_increment_);
    }

    void SetRatioToSpeedOfSound(TimeType ratio) {
        use_default_speed_ = false;
        delay_per_sample_increment_ = std::floor(ratio * 1000) / 1000.0f;
    }

    void UseDefaultSpeed(bool use_default_speed) {
        if (use_default_speed == use_default_speed_)
            return;
        use_default_speed_.store(use_default_speed);

        if (use_default_speed) {
            delay_per_sample_increment_ = (delay_target_ - current_delay_) / kDelayInterpolationSteps;
        } else {
            delay_per_sample_increment_ = delay_target_ >= current_delay_ ? std::abs(delay_per_sample_increment_)
                                                                          : -std::abs(delay_per_sample_increment_);
        }
    }

    void Process(const DataType* input, DataType* output, size_t size, bool is_accumulative = false) {
        if (is_accumulative) {
            for (int sample = 0; sample < size; ++sample) {
                TapIn(input[sample]);
                output[sample] += GetDelayedSample();
            }
        } else {
            for (int sample = 0; sample < size; ++sample) {
                TapIn(input[sample]);
                output[sample] = GetDelayedSample();
            }
        }
    }

    DataType GetDelayedSample(int offset = 0) {
        current_delay_floor_ = static_cast<size_t>(std::floor(current_delay_));
        DataType fractional_factor = static_cast<DataType>(current_delay_ - std::floor(current_delay_));
        TimeType delta = std::abs(delay_target_ - current_delay_);
        if (delta > std::numeric_limits<TimeType>::epsilon() && delta > std::abs(delay_per_sample_increment_))
            current_delay_ = current_delay_ + delay_per_sample_increment_;
        else
            current_delay_ = delay_target_.load();
        return (1 - fractional_factor) * ring_buffer_[-current_delay_floor_ + offset] +
               fractional_factor * ring_buffer_[-current_delay_floor_ - 1 + offset];
    }

    void TapIn(DataType value) {
        ring_buffer_.Write(&value, 1);
        ring_buffer_.OffsetReadIndex(1);
    }

    TimeType current_delay() const {
        return current_delay_;
    }

    TimeType delay_per_sample_increment() const {
        return delay_per_sample_increment_;
    }

    int delay_interpolation_steps() const {
        return kDelayInterpolationSteps;
    }

private:
    static DataType GetDefaultElementValue();
    RingBuffer<DataType> ring_buffer_;
    std::atomic<TimeType> delay_target_;
    std::atomic<TimeType> current_delay_;
    size_t current_delay_floor_{};
    std::atomic<TimeType> delay_per_sample_increment_;

    std::atomic<bool> use_default_speed_;
    static constexpr int kDelayInterpolationSteps = 2048;
};

}  // namespace avs3renderer
