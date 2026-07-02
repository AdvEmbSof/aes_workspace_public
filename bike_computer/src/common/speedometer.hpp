// Copyright 2025 Haute école d'ingénierie et d'architecture de Fribourg
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/****************************************************************************
 * @file speedometer.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Speedometer header file
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

// std
#include <chrono>

// local
#include "constants.hpp"

// zpp_lib
#include "zpp_include/mutex.hpp"
#include "zpp_include/non_copyable.hpp"
#include "zpp_include/thread.hpp"

// stl
#if CONFIG_TEST
#include <functional>
#endif  // CONFIG_TEST

namespace bike_computer {

using std::literals::chrono_literals::operator""ms;
using std::literals::chrono_literals::operator""us;

class Speedometer : private zpp_lib::NonCopyable {
public:
  Speedometer();
  ~Speedometer() = default;

  // method used for setting the current pedal rotation time
  void set_current_pedal_rotation_time(const std::chrono::milliseconds& current_rotation_time);

  // method used for setting/getting the current gear
  void set_gear_size(uint8_t gear_size);

  // method called for getting the current speed (expressed in km / h)
  float get_current_speed() const;

  // method called for getting the current traveled distance (expressed in km)
  float get_traveled_distance();

  // method called for resetting the traveled distance
  void reset();

  // methods used for tests only
#if CONFIG_TEST == 1
  uint8_t get_gear_size() const;
  float get_wheel_circumference() const;
  uint8_t get_tray_size() const;
  std::chrono::milliseconds get_current_pedal_rotation_time() const;
  using CallbackFunction = std::function<void()>;
  void set_on_reset_callback(CallbackFunction cb);
#endif  // CONFIG_TEST == 1

private:
  // private methods
  void compute_speed();
  float compute_traveled_distance();

  // definition of task period time
  static constexpr std::chrono::milliseconds kTaskPeriod = 400ms;
  // definition of task execution time
  static constexpr std::chrono::microseconds kTaskRunTime = 200000us;

  // constants related to speed computation
  static constexpr float kWheelCircumference     = 2.1f;
  static constexpr uint8_t kTraySize             = 50;
  std::chrono::microseconds _last_time           = std::chrono::microseconds::zero();
  std::chrono::milliseconds _pedal_rotation_time = kInitialPedalRotationTime;

  // data members
  // LowPowerTicker _ticker;
  float _current_speed = 0.0f;
  zpp_lib::Mutex _total_distance_mutex;
  float _total_distance = 0.0f;
  uint8_t _gear_size    = 1;

#if CONFIG_TEST == 1
  std::function<void()> _cb;
#endif  // CONFIG_TEST == 1
};

}  // namespace bike_computer
