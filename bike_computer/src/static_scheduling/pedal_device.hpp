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
 * @file pedal_device.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief PedalDevice header file (static scheduling)
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

// local
#include "common/constants.hpp"

// zpp_lib
#include "zpp_include/interrupt_in.hpp"
#include "zpp_include/non_copyable.hpp"

namespace bike_computer::static_scheduling {

class PedalDevice : private zpp_lib::NonCopyable {
public:
  PedalDevice();

  // method called for updating the bike system
  std::chrono::milliseconds get_current_rotation_time();

private:
  // private methods
  void increase_rotation_speed();
  void decrease_rotation_speed();

  // data members
  std::chrono::milliseconds _pedal_rotation_time = bike_computer::kInitialPedalRotationTime;

  // buttons
  zpp_lib::InterruptIn _button2;
  zpp_lib::InterruptIn _button3;
  zpp_lib::InterruptIn _button4;
};

}  // namespace bike_computer::static_scheduling
