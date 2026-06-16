// stl
#include <chrono>

// zpp-lib
#include "zpp_include/thread.hpp"
#include "zpp_include/this_thread.hpp"
#include "zpp_include/digital_out.hpp"

// zephyr
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

LOG_MODULE_REGISTER(sensor_bm280, CONFIG_APP_LOG_LEVEL);

void read_sensor() {

	const struct device* _sensorDevice = DEVICE_DT_GET(DT_INST(0, bosch_bme280));
	using namespace std::literals;
	static std::chrono::milliseconds readInterval = 1000ms;

	if (!device_is_ready(_sensorDevice)){
		LOG_ERR("Device %s not found", _sensorDevice->name);
		return;
	}

	struct sensor_value temperature_sv, humidity_sv, pressure_sv;

	while (true) {
		sensor_sample_fetch(_sensorDevice);

  		sensor_channel_get(_sensorDevice, SENSOR_CHAN_AMBIENT_TEMP, &temperature_sv);
  		sensor_channel_get(_sensorDevice, SENSOR_CHAN_HUMIDITY, &humidity_sv);
  		sensor_channel_get(_sensorDevice, SENSOR_CHAN_PRESS, &pressure_sv);

		LOG_INF("T=%.2f [deg C] P=%.2f [kPa] H=%.1f [%%]",
                sensor_value_to_double(&temperature_sv),
                sensor_value_to_double(&pressure_sv),
                sensor_value_to_double(&humidity_sv));

		zpp_lib::ThisThread::sleep_for(readInterval);
	}
}

int main(void) {
	
    LOG_DBG("Running on board %s", CONFIG_BOARD_TARGET);

	zpp_lib::Thread thread(zpp_lib::PreemptableThreadPriority::PriorityNormal, "Blinky");
    auto res = thread.start(read_sensor);
	if (! res) {
		return -1;
	}

	res = thread.join();
	if (! res) {
		LOG_ERR("Could not join thread: %d", (int) res.error());
		return -1;
	}

    return 0;
}