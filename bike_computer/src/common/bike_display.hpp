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
 * @file bike_display.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Bike Display header file
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

// zpp_lib
#include "zpp_include/zephyr_result.hpp"

namespace bike_computer {

#if CONFIG_DISPLAY == 1

class BikeDisplay {
 public:
  // constructor
  BikeDisplay() = default;

  // to be called prior to any other method
  zpp_lib::ZephyrResult initialize();

  void displayGear(uint8_t gear);
  void displaySpeed(float speed);
  void displayDistance(float distance);
  void displayTemperature(float temperature);
  void reset();

 private:
  // private methods
  void displayTitle();
  void displayInfo();
  void displayIcons();
  void drawLines();
  void computePositions();
  void drawVerticalLine(uint32_t color, uint32_t xPos, uint32_t width);
  void drawHorizontalLine(uint32_t color, uint32_t yPos, uint32_t width);

  static constexpr uint32_t kLineWidth   = 2;
  static constexpr uint32_t kIconXMargin = 20;
#if CONFIG_SHIELD_ADAFRUIT_2_8_TFT_TOUCH_V2 == 1
  static constexpr uint32_t kTextXMargin = 30;
#else
  static constexpr uint32_t kTextXMargin = 40;
#endif
  static constexpr uint8_t kSpeedometerIndex = 0;
  static constexpr uint8_t kGearIndex        = 1;
  static constexpr uint8_t kTemperatureIndex = 2;
  static constexpr uint8_t kDistanceIndex    = 3;
  uint32_t _horLineYPos                      = 0;
  uint32_t _vertLineXPos                     = 0;
  uint32_t _infoBoxHeight                    = 0;
  uint32_t _speedometerIconXPos              = 0;
  uint32_t _speedometerIconYPos              = 0;
  uint32_t _speedometerTextMidXPos           = 0;
  uint32_t _speedometerTextYPos              = 0;
  uint32_t _gearIconXPos                     = 0;
  uint32_t _gearIconYPos                     = 0;
  uint32_t _gearTextMidXPos                  = 0;
  uint32_t _gearTextYPos                     = 0;
  uint32_t _temperatureIconXPos              = 0;
  uint32_t _temperatureIconYPos              = 0;
  uint32_t _temperatureTextMidXPos           = 0;
  uint32_t _temperatureTextYPos              = 0;
  uint32_t _distanceIconXPos                 = 0;
  uint32_t _distanceIconYPos                 = 0;
  uint32_t _distanceTextMidXPos              = 0;
  uint32_t _distanceTextYPos                 = 0;
};

#else
// default dummy BikeDisplay
class BikeDisplay {
 public:
  // constructor
  BikeDisplay() = default;
  zpp_lib::ZephyrResult initialize() { return zpp_lib::ZephyrResult(); }
  void displayGear(uint8_t gear) {}
  void displaySpeed(float speed) {}
  void displayDistance(float distance) {}
  void displayTemperature(float temperature) {}
};

#endif  // CONFIG_DISPLAY == 1

}  // namespace bike_computer
