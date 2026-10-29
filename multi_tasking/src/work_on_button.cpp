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
 * @file work_on_button.cpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Implementation of the WorkOnButton class
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/
#include "work_on_button.hpp"

// zephyr

// zpp_lib
#include "zpp_include/this_thread.hpp"
#include "zpp_include/time.hpp"
#include "zpp_include/utils.hpp"
#include "zpp_include/zpp_assert.hpp"
#include "zpp_include/zpp_log.hpp"

ZPP_LOG_MODULE_DECLARE(multi_tasking, CONFIG_APP_LOG_LEVEL);

namespace multi_tasking {

WorkOnButton::WorkOnButton(const char* threadName)
    : _thread(zpp_lib::PreemptableThreadPriority::PriorityNormal, threadName),
      _push_button_token(_push_button.add_callback([this]() { this->on_button_pressed(); }))
#if CONFIG_WORK_ON_BUTTON_USE_WORK_QUEUE
      ,
      _work_queue("ProcessingWorkQueue", zpp_lib::PreemptableThreadPriority::PriorityAboveNormal),
      _work(zpp_lib::Work<WorkOnButton>(this, &WorkOnButton::do_processing_with_cs))
#endif  // CONFIG_WORK_ON_BUTTON_USE_WORK_QUEUE
{
}

zpp_lib::ZephyrResult WorkOnButton::start() {
  auto res = _thread.start([this]() { this->perform_work(); });
  if (!res) {
    ZPP_LOG_ERR("Failed to start thread: %d", static_cast<int>(res.error()));
    return res;
  }
  printk("Thread started successfully\n");
  return res;
}

void WorkOnButton::perform_work() {
  zpp_lib::Utils::log_threads_summary();
  // executes in thread context
  while (true) {
    // processing within cs
    do_processing_with_cs();

    // do some processing outside the critial section
    printk("Performing processing outside CS, thread %p\n", static_cast<void*>(k_current_get()));
#if CONFIG_WORK_ON_BUTTON_SLEEP
    zpp_lib::ThisThread::sleep_for(kProcessingTime);
#else
    // busy waiting for the button to be pressed
    zpp_lib::ThisThread::busy_wait(kProcessingTime);
#endif
  }
}

void WorkOnButton::wait_exit() {
  auto res = _thread.join();
  if (!res) {
    ZPP_LOG_ERR("join() failed: %d", static_cast<int>(res.error()));
  }
}

void WorkOnButton::do_processing_with_cs() {
  // do some processing within the critial section
  auto res = _mutex.lock();
  ZPP_ASSERT(res, "Cannot lock mutex: %d", static_cast<int>(res.error()));

  printk("Performing processing in CS, thread (%p)\n", static_cast<void*>(k_current_get()));
#if CONFIG_WORK_ON_BUTTON_SLEEP
  zpp_lib::ThisThread::sleep_for(kProcessingTime);
#else
  // busy waiting for the button to be pressed
  zpp_lib::ThisThread::busy_wait(kProcessingTime);
#endif

  res = _mutex.unlock();
  ZPP_ASSERT(res, "Cannot unlock mutex: %d", static_cast<int>(res.error()));
}

void WorkOnButton::on_button_pressed() {
#if CONFIG_WORK_ON_BUTTON_USE_WORK_QUEUE
  // schedule the work item to be executed in thread context
  auto res = _work_queue.call(_work);
  if (!res) {
    ZPP_LOG_ERR("Cannot call work in on_button_pressed: %d", static_cast<int>(res.error()));
  }
#else
  // execute the work in ISR context
  do_processing_with_cs();
#endif
}

}  // namespace multi_tasking
