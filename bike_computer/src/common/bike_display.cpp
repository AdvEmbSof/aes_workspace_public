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
 * @file bike_display.cpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Bike Display header file
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#include "bike_display.hpp"

// zephyr
#include <zephyr/logging/log.h>

// std
#include <cstdio>

// zpp_lib
#include "zpp_include/display.hpp"

// icons and fonts
#if CONFIG_SHIELD_ADAFRUIT_2_8_TFT_TOUCH_V2 == 1
#include "resources/celsius_icon_20.hpp"
#include "resources/distance_icon_50.hpp"
#include "resources/gear_icon_50.hpp"
#include "resources/speedometer_icon_50.hpp"
#include "resources/thermometer_icon_50.hpp"
#else
#include "resources/celsius_icon_32.hpp"
#include "resources/distance_icon_100.hpp"
#include "resources/gear_icon_100.hpp"
#include "resources/speedometer_icon_100.hpp"
#include "resources/thermometer_icon_100.hpp"
#endif
#include "resources/fonts.hpp"

LOG_MODULE_DECLARE(bike_computer, CONFIG_APP_LOG_LEVEL);

namespace bike_computer {

#if CONFIG_DISPLAY == 1

// create the Display instance as a global variable (not on stack)
static zpp_lib::Display gDisplay;

// constants
static constexpr uint32_t DISPLAY_COLOR_BLUE  = 0xFF0000FFUL;
static constexpr uint32_t DISPLAY_COLOR_WHITE = 0xFFFFFFFFUL;
static constexpr uint32_t DISPLAY_COLOR_BLACK = 0x00000000UL;
#if CONFIG_SHIELD_ADAFRUIT_2_8_TFT_TOUCH_V2 == 1
static constexpr uint32_t kTitleHeight = 60;
#else
static constexpr uint32_t kTitleHeight = 112;
#endif

// type definitions for logos
struct Logos {
  struct ImageInfo {
    const uint32_t* pImageData;
    const uint8_t imageWidth;
    const uint8_t imageHeight;
  };

  static constexpr uint8_t kNbrOfImages               = 4;
  static constexpr ImageInfo _imageInfo[kNbrOfImages] = {
      {.pImageData  = speedometer_icon,
       .imageWidth  = kSpeedometerIconWidth,
       .imageHeight = kSpeedometerIconHeight},
      {.pImageData  = gear_icon,
       .imageWidth  = kGearIconWidth,
       .imageHeight = kGearIconHeight},
      {.pImageData  = thermometer_icon,
       .imageWidth  = kThermometerIconWidth,
       .imageHeight = kThermometerIconHeight},
      {.pImageData  = distance_icon,
       .imageWidth  = kDistanceIconWidth,
       .imageHeight = kDistanceIconHeight}};
};
static const Logos gLogos;

zpp_lib::ZephyrResult BikeDisplay::initialize() {
  // initialize the display
  auto res = gDisplay.initialize();
  if (!res) {
    LOG_ERR("Failed to initialized display: %d", (int)res.error());
    return res;
  } else {
    LOG_DBG("Display initialized");
  }

  // compute positions
  computePositions();

  // display title
  displayTitle();

  // display icons (with lines)
  displayIcons();

  // setup font for printing bike info
  gDisplay.setBackColor(DISPLAY_COLOR_WHITE);
  gDisplay.setTextColor(DISPLAY_COLOR_BLUE);

  return res;
}

void BikeDisplay::computePositions() {
  // horizontal and vertical lines below title bar
  _vertLineXPos  = gDisplay.getWidth() / 2;
  _infoBoxHeight = (gDisplay.getHeight() - kTitleHeight);
  _horLineYPos   = kTitleHeight + _infoBoxHeight / 2;
  // speed (top left)
  _speedometerIconXPos = kIconXMargin;
  _speedometerIconYPos = _horLineYPos - _infoBoxHeight / 4 -
                         gLogos._imageInfo[kSpeedometerIndex].imageHeight / 2;
  const uint32_t speedoTextBoxWidth = _vertLineXPos - _speedometerIconXPos;
  _speedometerTextMidXPos = _speedometerIconXPos + speedoTextBoxWidth / 2 + kTextXMargin;
  _speedometerTextYPos    = _horLineYPos - _infoBoxHeight / 4;
  // distance (bottom left)
  _distanceIconXPos = kIconXMargin;
  _distanceIconYPos = _horLineYPos + _infoBoxHeight / 4 -
                      gLogos._imageInfo[kDistanceIndex].imageHeight / 2;
  const uint32_t distTextBoxWidth = _vertLineXPos - _distanceIconXPos;
  _distanceTextMidXPos = _distanceIconXPos + distTextBoxWidth / 2 + kTextXMargin;
  _distanceTextYPos    = _horLineYPos + _infoBoxHeight / 4;
  // temperature (top right)
  _temperatureIconXPos = _vertLineXPos + kIconXMargin;
  _temperatureIconYPos = _horLineYPos - _infoBoxHeight / 4 -
                         gLogos._imageInfo[kTemperatureIndex].imageHeight / 2;
  const uint32_t tempTextBoxWidth = gDisplay.getWidth() - _temperatureIconXPos;
  _temperatureTextMidXPos =
      _temperatureIconXPos - kCelsiusIconWidth + tempTextBoxWidth / 2 + kTextXMargin;
  _temperatureTextYPos = _horLineYPos - _infoBoxHeight / 4;
  // gear (bottom right)
  _gearIconXPos = _vertLineXPos + kIconXMargin;
  _gearIconYPos =
      _horLineYPos + _infoBoxHeight / 4 - gLogos._imageInfo[kGearIndex].imageHeight / 2;
  const uint32_t gearTextBoxWidth = gDisplay.getWidth() - _gearIconXPos;
  _gearTextMidXPos                = _gearIconXPos + gearTextBoxWidth / 2 + kTextXMargin;
  _gearTextYPos                   = _horLineYPos + _infoBoxHeight / 4;
}

void BikeDisplay::displayTitle() {
  gDisplay.fillDisplay(DISPLAY_COLOR_WHITE);
  gDisplay.setTextColor(DISPLAY_COLOR_BLUE);
  gDisplay.fillRectangle(DISPLAY_COLOR_BLUE, 0, 0, gDisplay.getWidth(), kTitleHeight);
  gDisplay.setBackColor(DISPLAY_COLOR_BLUE);
  gDisplay.setTextColor(DISPLAY_COLOR_WHITE);
  gDisplay.setFont(getFont18());
  gDisplay.drawStringAtLine(1, "Bike Computer", zpp_lib::Display::AlignMode::CENTER_MODE);
}

void BikeDisplay::displayIcons() {
  // draw the vertical and horizontal lines
  drawVerticalLine(DISPLAY_COLOR_BLUE, _vertLineXPos, kLineWidth);
  drawHorizontalLine(DISPLAY_COLOR_BLUE, _horLineYPos, kLineWidth);

  // draw the speedometer icon
  gDisplay.drawPicture(
      _speedometerIconXPos,
      _speedometerIconYPos,
      const_cast<uint32_t*>(gLogos._imageInfo[kSpeedometerIndex].pImageData),
      gLogos._imageInfo[kSpeedometerIndex].imageWidth,
      gLogos._imageInfo[kSpeedometerIndex].imageHeight);

  // draw the distance icon
  gDisplay.drawPicture(
      _distanceIconXPos,
      _distanceIconYPos,
      const_cast<uint32_t*>(gLogos._imageInfo[kDistanceIndex].pImageData),
      gLogos._imageInfo[kDistanceIndex].imageWidth,
      gLogos._imageInfo[kDistanceIndex].imageHeight);

  // draw the temperature icon
  gDisplay.drawPicture(
      _temperatureIconXPos,
      _temperatureIconYPos,
      const_cast<uint32_t*>(gLogos._imageInfo[kTemperatureIndex].pImageData),
      gLogos._imageInfo[kTemperatureIndex].imageWidth,
      gLogos._imageInfo[kTemperatureIndex].imageHeight);

  // draw the gear icon
  gDisplay.drawPicture(_gearIconXPos,
                       _gearIconYPos,
                       const_cast<uint32_t*>(gLogos._imageInfo[kGearIndex].pImageData),
                       gLogos._imageInfo[kGearIndex].imageWidth,
                       gLogos._imageInfo[kGearIndex].imageHeight);
}

void BikeDisplay::displayGear(uint8_t gear) {
  char msg[10] = {0};
  int strlen   = snprintf(msg, sizeof(msg), "%d", gear);
#if CONFIG_SHIELD_ADAFRUIT_2_8_TFT_TOUCH_V2 == 1
  gDisplay.setFont(getFont18());
#else
  gDisplay.setFont(getFont36b());
#endif
  const uint32_t msgLen   = strlen * gDisplay.getFont()->width;
  const uint32_t textXPos = _gearTextMidXPos - msgLen / 2;
  const uint32_t textYPos = _gearTextYPos - gDisplay.getFont()->height / 2;
  gDisplay.drawStringAt(textXPos, textYPos, msg, zpp_lib::Display::AlignMode::LEFT_MODE);
}

void BikeDisplay::displaySpeed(float speed) {
  char msg[10] = {0};
  int strlen   = snprintf(msg, sizeof(msg), "%.1f", static_cast<double>(speed));
#if CONFIG_SHIELD_ADAFRUIT_2_8_TFT_TOUCH_V2 == 1
  gDisplay.setFont(getFont16());
#else
  gDisplay.setFont(getFont18());
#endif
  const uint32_t msgLen   = strlen * gDisplay.getFont()->width;
  const uint32_t textXPos = _speedometerTextMidXPos - msgLen / 2;
  const uint32_t textYPos = _speedometerTextYPos - gDisplay.getFont()->height / 2;
  gDisplay.drawStringAt(textXPos, textYPos, msg, zpp_lib::Display::AlignMode::LEFT_MODE);
}

void BikeDisplay::displayDistance(float distance) {
  char msg[10] = {0};
  int strlen   = snprintf(msg, sizeof(msg), "%.2f", static_cast<double>(distance));
#if CONFIG_SHIELD_ADAFRUIT_2_8_TFT_TOUCH_V2 == 1
  gDisplay.setFont(getFont16());
#else
  gDisplay.setFont(getFont18());
#endif
  const uint32_t msgLen   = strlen * gDisplay.getFont()->width;
  const uint32_t textXPos = _distanceTextMidXPos - msgLen / 2;
  const uint32_t textYPos = _distanceTextYPos - gDisplay.getFont()->height / 2;
  gDisplay.drawStringAt(textXPos, textYPos, msg, zpp_lib::Display::AlignMode::LEFT_MODE);
}

void BikeDisplay::displayTemperature(float temperature) {
  char msg[10] = {0};
  int strlen   = snprintf(msg, sizeof(msg), "%.1f", static_cast<double>(temperature));
#if CONFIG_SHIELD_ADAFRUIT_2_8_TFT_TOUCH_V2 == 1
  gDisplay.setFont(getFont16());
#else
  gDisplay.setFont(getFont26b());
#endif
  const uint32_t msgLen   = strlen * gDisplay.getFont()->width;
  const uint32_t textXPos = _temperatureTextMidXPos - msgLen / 2;
  const uint32_t textYPos = _temperatureTextYPos - gDisplay.getFont()->height / 2;
  gDisplay.drawStringAt(textXPos, textYPos, msg, zpp_lib::Display::AlignMode::LEFT_MODE);
  const uint32_t celsiusIconXPos = textXPos + msgLen;
  const uint32_t celsiusIconYPos = textYPos - kCelsiusIconHeight / 5;
  gDisplay.drawPicture(celsiusIconXPos,
                       celsiusIconYPos,
                       const_cast<uint32_t*>(celsius_icon),
                       kCelsiusIconWidth,
                       kCelsiusIconHeight);
}

void BikeDisplay::drawVerticalLine(uint32_t color, uint32_t xPos, uint32_t width) {
  for (uint32_t i = kTitleHeight; i < gDisplay.getHeight(); i++) {
    gDisplay.fillRectangle(color, xPos, i, width, 1);
  }
}

void BikeDisplay::drawHorizontalLine(uint32_t color, uint32_t yPos, uint32_t width) {
  gDisplay.fillRectangle(color, 0, yPos, gDisplay.getWidth(), width);
}

#endif  // CONFIG_DISPLAY == 1

}  // namespace bike_computer
