// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <Arduino.h>
#include <helpers/ESP32Board.h>
#include <driver/rtc_io.h>
#include "INA219Driver.h"
#include "INA226Driver.h"

#ifndef ADC_MULTIPLIER
  #define ADC_MULTIPLIER 1.0
#endif

class AttakyMeshSeriesBoard : public ESP32Board {

protected:
  float adc_mult = ADC_MULTIPLIER;
  // Fuel-gauge readout cache: getBattMilliVolts() is polled from the UI render /
  // mesh-telemetry path, so the actual I2C read is throttled to ~2 s and the
  // last value is served in between (0 = not yet read). The gauge chip
  // (INA226, INA219 or MAX17048) is latched only once it has answered; until
  // then probing repeats, throttled by batt_probe_ms.
  enum GaugeKind : uint8_t { GAUGE_UNKNOWN, GAUGE_INA226, GAUGE_INA219, GAUGE_MAX17048 };
  uint16_t batt_cache_mv = 0;
  uint32_t batt_last_ms  = 0;
  INA226Driver ina226;
  INA219Driver ina219;
  GaugeKind gauge_kind   = GAUGE_UNKNOWN;
  bool     batt_probed   = false;
  uint32_t batt_probe_ms = 0;

public:
  AttakyMeshSeriesBoard() { }

  void begin();
  void onBeforeTransmit(void) override;
  void onAfterTransmit(void) override;
  void enterDeepSleep(uint32_t secs, int pin_wake_btn = -1);
  void powerOff() override;
  uint16_t getBattMilliVolts() override;
  bool setAdcMultiplier(float multiplier) override {
    if (multiplier == 0.0f) {
      adc_mult = ADC_MULTIPLIER;
    } else {
      adc_mult = multiplier;
    }
    return true;
  }
  float getAdcMultiplier() const override { return adc_mult; }
  const char* getManufacturerName() const override;
};
