// Copyright 2022 Haute école d'ingénierie et d'architecture de Fribourg
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
//
// @file wait_on_button.hpp
// @author Serge Ayer
// @brief WaitOnButton class used for demonstrating Events
// @date 2025-10-21
// @version 0.1.0

#pragma once

#include "zpp_include/thread.hpp"
#include "zpp_include/events.hpp"
#include "zpp_include/interrupt_in.hpp"

#include <chrono>

namespace multi_tasking {

class WaitOnButton {
public:
    explicit WaitOnButton(const char* threadName);

    void start();
    void wait_started();
    void wait_exit();

private:
    void waitForButtonEvent();
    void buttonPressed();

    static constexpr uint8_t kPressedEventFlag = BIT(0);
    static constexpr uint8_t kStartedEventFlag = BIT(1);
    
    zpp_lib::Thread _thread;
    std::chrono::microseconds _pressedTime;
    zpp_lib::Events _eventFlags;
    zpp_lib::InterruptIn<zpp_lib::PinName::BUTTON1> _pushButton;
};

} // namespace multi_tasking
