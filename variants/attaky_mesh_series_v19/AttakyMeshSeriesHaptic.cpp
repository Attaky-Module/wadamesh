// SPDX-License-Identifier: GPL-3.0-or-later
#include "AttakyMeshSeriesHaptic.h"

#if defined(ATTAKY_MESH_SERIES) && defined(HAS_ATTAKY_MESH_MOTOR) && defined(ESP32)

#include <Arduino.h>
#include <Wire.h>
#include "AttakySharedI2C.h"

// Core expander AW9523 @0x59, port 1 bit 4 (P14): BTB pin 11 "PA14" ->
// driver pin 12, active high. Register map per the AW9523 datasheet (and the
// touch driver, which owns P13 on the same port-1 registers).
static const uint8_t HAPTIC_ADDR    = 0x59;
static const uint8_t AW_REG_OUT_P1  = 0x03;
static const uint8_t AW_REG_CFG_P1  = 0x05;
static const uint8_t AW_REG_MODE_P1 = 0x13;        // 0x13 = P1 LED-mode select

static const uint8_t MOTOR_BIT      = (1u << 4);   // P14

static const uint32_t NOTIFY_PULSE_MS = 250;

static const uint32_t INIT_RETRY_MS = 500;

static bool s_inited  = false;   // set only once P14 is confirmed parked
static uint32_t s_next_init_ms = 0;
static volatile bool     s_motor_on = false;
static volatile uint32_t s_off_ms   = 0;

static bool awRead(uint8_t reg, uint8_t* val) {
  Wire.beginTransmission(HAPTIC_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)HAPTIC_ADDR, 1) != 1) return false;
  *val = (uint8_t)Wire.read();
  return true;
}

static bool awWrite(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(HAPTIC_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

// Read-modify-write a single bit. Port 1 is shared with the touch driver's
// CTP-reset line (P13), so whole-register writes would clobber it.
static bool awRmw(uint8_t reg, uint8_t bitmask, bool set) {
  uint8_t v = 0;
  if (!awRead(reg, &v)) return false;
  if (set) v |= bitmask; else v &= (uint8_t)~bitmask;
  return awWrite(reg, v);
}

void attakyHapticInit() {
  if (s_inited) return;
  s_next_init_ms = millis() + INIT_RETRY_MS;
  if (!attakyI2cLock(30)) return;   // retried from attakyHapticPoll()
  // P14: GPIO mode (LED-mode bit set), push-pull output (config bit clear),
  // driven LOW. Parking is mandatory: the port is high-Z out of POR and a
  // floating drive line keeps the motor running from power-on. Latched only
  // when every write landed; each step is idempotent so a retry is safe.
  bool ok = awRmw(AW_REG_MODE_P1, MOTOR_BIT, true);
  ok = ok && awRmw(AW_REG_CFG_P1,  MOTOR_BIT, false);
  ok = ok && awRmw(AW_REG_OUT_P1,  MOTOR_BIT, false);
  s_inited = ok;
  attakyI2cUnlock();
}

void attakyHapticNotifyPulse() {
  if (!s_inited || s_motor_on) return;
  if (!attakyI2cLock(10)) return;   // bus busy -> skip rather than block the UI
  if (awRmw(AW_REG_OUT_P1, MOTOR_BIT, true)) {
    s_motor_on = true;
    s_off_ms = millis() + NOTIFY_PULSE_MS;
  }
  attakyI2cUnlock();
}

void attakyHapticPoll() {
  if (!s_inited) {
    if ((int32_t)(millis() - s_next_init_ms) >= 0) attakyHapticInit();
    return;
  }
  if (!s_motor_on) return;
  if ((int32_t)(millis() - s_off_ms) < 0) return;
  if (!attakyI2cLock(10)) return;   // retry next loop
  // Clear the running flag only once the line is actually LOW again.
  if (awRmw(AW_REG_OUT_P1, MOTOR_BIT, false)) s_motor_on = false;
  attakyI2cUnlock();
}

#endif
