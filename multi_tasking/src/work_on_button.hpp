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
 * @file work_on_button.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Declaration of the WorkOnButton class
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

// stl
#include <atomic>
#include <chrono>

// zpp_lib
#include "zpp_include/interrupt_in.hpp"
#include "zpp_include/thread.hpp"
#include "zpp_include/work.hpp"
#include "zpp_include/work_queue.hpp"

namespace multi_tasking {

using std::literals::chrono_literals::operator""s;

class WorkOnButton {
public:
  explicit WorkOnButton(const char* threadName);

  [[nodiscard]] zpp_lib::ZephyrResult start();
  void wait_exit();

private:
  void perform_work();
  void do_processing_with_cs();
  void on_button_pressed();

  static constexpr auto kProcessingTime = 5s;

  zpp_lib::Mutex _mutex;
  zpp_lib::Thread _thread;
  zpp_lib::InterruptIn _push_button{zpp_lib::InterruptIn::PinName::BUTTON1};
  zpp_lib::RegistrationToken _push_button_token;
#if CONFIG_WORK_ON_BUTTON_USE_WORK_QUEUE
  zpp_lib::WorkQueue _work_queue;
  zpp_lib::Work<WorkOnButton> _work{zpp_lib::Work<WorkOnButton>(this, &WorkOnButton::perform_work)};
#endif  // CONFIG_WORK_ON_BUTTON_USE_WORK_QUEUE
};

}  // namespace multi_tasking
