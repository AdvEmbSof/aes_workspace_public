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

namespace bike_computer {

namespace static_scheduling {

class PedalDevice : private zpp_lib::NonCopyable<PedalDevice> {
 public:
  PedalDevice() = default;

  // method called for updating the bike system
  std::chrono::milliseconds getCurrentRotationTime();

 private:
  // private methods
  void increaseRotationSpeed();
  void decreaseRotationSpeed();

  // data members
  std::chrono::milliseconds _pedalRotationTime = bike_computer::kInitialPedalRotationTime;

  // buttons
  zpp_lib::InterruptIn<zpp_lib::PinName::BUTTON2> _button2;
  zpp_lib::InterruptIn<zpp_lib::PinName::BUTTON3> _button3;
  zpp_lib::InterruptIn<zpp_lib::PinName::BUTTON4> _button4;
};

}  // namespace static_scheduling

}  // namespace bike_computer
