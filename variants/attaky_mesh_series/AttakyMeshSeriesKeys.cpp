// SPDX-License-Identifier: GPL-3.0-or-later
#include "AttakyMeshSeriesKeys.h"

#if defined(ATTAKY_MESH_SERIES) && defined(ESP32)

#include <Arduino.h>
#include <Wire.h>
#include "AttakySharedI2C.h"

static const uint8_t KEYS_ADDR      = 0x59;

static const uint8_t AW_REG_IN_P0   = 0x00;
static const uint8_t AW_REG_CFG_P0  = 0x04;
static const uint8_t AW_REG_MODE_P0 = 0x12;   // 0x12 = P0 LED-mode select (0x13 is P1)

static const uint8_t POWER_BIT      = (1u << 7);   // P07 = POWER_BTN

// A press must read stable across this many consecutive polls before it counts,
// the sample that first saw the change included. The buttons sit behind an I2C
// expander on a shared bus, so a single sample is not enough to tell a real press
// from a marginal contact.
static const uint8_t  DEBOUNCE_POLLS = 3;   // 3 x 30 ms
static const uint32_t POLL_INTERVAL_MS = 30;

static bool     s_present  = false;
static uint32_t s_next_ms  = 0;

static bool    s_power_down  = false;   // debounced state
static uint8_t s_pending_n   = 0;       // consecutive polls agreeing with s_pending_down
static bool    s_pending_down = false;
static volatile bool s_power_event = false;

static bool awWrite(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(KEYS_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

// Reads IN_P0. Returns false on any I2C failure WITHOUT touching *val — a failed
// read must never be mistaken for 0xFF ("nothing pressed"), which is what an
// absent or marginally-connected expander also looks like.
static bool awReadInP0(uint8_t* val) {
  Wire.beginTransmission(KEYS_ADDR);
  Wire.write(AW_REG_IN_P0);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)KEYS_ADDR, 1) != 1) return false;
  *val = (uint8_t)Wire.read();
  return true;
}

// Configures the expander, and reports whether it is answering. Runs on every poll
// until it succeeds: there is no "initialised but dead" state to get stuck in.
// Nothing here latches a failure, because the failure this guards against is not
// an absent expander — P07 is soldered to this board — but a NACK from one write
// while the touch driver has the shared bus mid-transaction. Latching that off
// costs the panel its only wake path (dark-screen touch is deliberately absorbed,
// see UITask::noteUserInput), i.e. a screen that cannot be turned back on until
// the device is rebooted. Retrying carries no cost worth capping for: the caller
// gates us to POLL_INTERVAL_MS, so a retry is one transaction where a successful
// poll would have run one anyway.
static bool ensureInit() {
  if (s_present) return true;
  if (!attakyI2cLock(30)) return false;   // bus busy — retry on the next poll
  // P0 as GPIO inputs. Both are the POR defaults, but the touch driver shares
  // this expander (it drives P13 for CTP_RESET), so state them explicitly rather
  // than depend on who initialised first.
  bool ok = awWrite(AW_REG_MODE_P0, 0xFF);   // GPIO mode, not LED mode
  ok &= awWrite(AW_REG_CFG_P0,  0xFF);       // all inputs
  s_present = ok;
  attakyI2cUnlock();
  return ok;
}

void attakyKeysPoll() {
  // Ahead of the init so a failing expander is re-probed at the poll interval
  // rather than on every pass of the UI loop.
  const uint32_t now = millis();
  if ((int32_t)(now - s_next_ms) < 0) return;
  s_next_ms = now + POLL_INTERVAL_MS;

  if (!ensureInit()) return;

  uint8_t p0 = 0xFF;
  bool ok = false;
  if (attakyI2cLock(10)) {
    ok = awReadInP0(&p0);
    attakyI2cUnlock();
  }
  if (!ok) return;   // bus busy or read failed — hold the previous state

  const bool down = (p0 & POWER_BIT) == 0;   // active low

  if (down != s_pending_down) { s_pending_down = down; s_pending_n = 1; return; }
  if (s_pending_n < DEBOUNCE_POLLS) s_pending_n++;   // saturates — never wraps while a key is held
  if (s_pending_n < DEBOUNCE_POLLS) return;

  if (down != s_power_down) {
    s_power_down = down;
    if (down) s_power_event = true;   // fire on the press edge
  }
}

bool attakyPowerKeyPressed() {
  if (!s_power_event) return false;
  s_power_event = false;
  return true;
}

bool attakyKeysPresent() { return s_present; }

#else

void attakyKeysPoll() {}
bool attakyPowerKeyPressed() { return false; }
bool attakyKeysPresent() { return false; }

#endif
