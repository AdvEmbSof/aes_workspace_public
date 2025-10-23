#include "wait_on_button.hpp"
#include <zephyr/logging/log.h>
#include "zpp_include/time.hpp"

LOG_MODULE_REGISTER(wait_on_button, CONFIG_APP_LOG_LEVEL);

namespace multi_tasking {

WaitOnButton::WaitOnButton(const char* threadName)
    : _thread(zpp_lib::PreemptableThreadPriority::PriorityNormal, threadName),
      _pressedTime(std::chrono::microseconds::zero())
{
    _pushButton.fall(std::bind(&WaitOnButton::buttonPressed, this));
    LOG_DBG("WaitOnButton initialized");
}

void WaitOnButton::start() {
    auto res = _thread.start(std::bind(&WaitOnButton::waitForButtonEvent, this));
   
    if (!res) {
        LOG_ERR("Failed to start thread: %d", (int)res.error());
        return;
    }
    LOG_DBG("Thread started successfully");
}

void WaitOnButton::wait_started() {
    _eventFlags.wait_any(kStartedEventFlag);
}

void WaitOnButton::wait_exit() {
    auto res = _thread.join();
    if (!res) {
        LOG_ERR("join() failed: %d", (int)res.error());
    }
}

void WaitOnButton::waitForButtonEvent() {
    LOG_DBG("Waiting for button press");
    _eventFlags.set(kStartedEventFlag);

    while (true) {
        _eventFlags.wait_any(kPressedEventFlag);
        std::chrono::microseconds time = zpp_lib::Time::getUpTime();
        std::chrono::microseconds latency = time - _pressedTime;
        LOG_DBG("Button pressed with response time: %lld usecs", latency.count());
        LOG_DBG("Waiting for button press");
    }
}

void WaitOnButton::buttonPressed() {
    _pressedTime = zpp_lib::Time::getUpTime();
    _eventFlags.set(kPressedEventFlag);
}

} // namespace multi_tasking
