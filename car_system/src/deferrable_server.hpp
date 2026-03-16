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
 * @file task_recorder.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief DeferrableServer class declaration
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

#if CONFIG_PHASE_B

// stl
#include <atomic>

// zpp_lib
#include "zpp_include/barrier.hpp"
#include "zpp_include/non_copyable.hpp"
#include "zpp_include/zephyr_result.hpp"

// local
#include "periodic_task_info.hpp"
#include "sporadic_task_generator.hpp"

namespace car_system {

class DeferrableServer : private zpp_lib::NonCopyable<DeferrableServer> {
 public:
  // method called from CarSystem::start() for starting generation of sporadic events
  void start(zpp_lib::Barrier& barrier,
             const PeriodicTaskInfo& taskInfo,
             SporadicTaskGenerator& taskGenerator);

  // method called for stopping the generator
  void stop();

 private:
  // stop flag, used for stopping each task (set in stop())
  volatile std::atomic<bool> _stopFlag = false;
};

}  // namespace car_system
#endif  // CONFIG_PHASE_B
