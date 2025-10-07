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
 * @file reset_device.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief ResetDevice header file (static scheduling)
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

// std
#include <chrono>

// zpp_lib
#include "zpp_include/interrupt_in.hpp"
#include "zpp_include/non_copyable.hpp"

namespace bike_computer {

namespace static_scheduling {

class ResetDevice : private zpp_lib::NonCopyable<ResetDevice> {
 public:
  ResetDevice();

  // method called for checking the reset status
  bool checkReset();

  // for computing the response time
  std::chrono::microseconds getPressTime();

 private:
  // called when one of the buttons is pressed
  void onFallButton1();

  // data members
  // We use two buttons that must be pressed together for reset
  zpp_lib::InterruptIn<zpp_lib::PinName::BUTTON1> _button1;
  std::chrono::microseconds _pressTime;
};

}  // namespace static_scheduling

}  // namespace bike_computer
