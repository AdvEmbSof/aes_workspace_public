// zephyr

#include <zephyr/kernel.h>

#include "zpp_include/utils.hpp"

#include "wait_on_button.hpp"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

int main(void) {  

  LOG_DBG("EventFlags program started");
  // log thread statistics
  zpp_lib::Utils::logThreadsSummary();
  // create the WaitOnButton instance and start it
  multi_tasking::WaitOnButton waitOnButton("Btn_th");
  waitOnButton.start();

  // wait that the WaitOnButton thread started 
  LOG_DBG("Calling wait_started()");
  waitOnButton.wait_started();
  LOG_DBG("wait_started() unblocked");

  // log thread statistics
  zpp_lib::Utils::logThreadsSummary();

  // wait for the thread to exit (will not because of infinite loop)
  //waitOnButton.wait_exit();
  // or do busy waiting
  while (true) {
    k_busy_wait(1000);
    // yield    
    k_usleep(10);
  }

  return 0;

}