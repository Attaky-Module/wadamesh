// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// LoRa Pager stack haptics: the vibration motor drive line is BTB pin 11 net
// "PA14" -> AW9523 (0x59) P14 -> motor driver input, ACTIVE HIGH (idle LOW).
// Same mapping as the FactoryTest "LoRa Pager 1W 1.0" motor test item.

#if defined(ATTAKY_MESH_SERIES) && defined(HAS_ATTAKY_MESH_MOTOR) && defined(ESP32)

/** Park P14 output-LOW. Call once at board bring-up: a floating drive line
 *  runs the motor continuously from power-on (the expander's ports stay
 *  high-Z until configured). Safe to call again; retries until the park
 *  writes have all succeeded. */
void attakyHapticInit();

/** Start one short notification vibration (no-op if already running or if
 *  the expander is absent). Non-blocking: the pulse is expired by
 *  attakyHapticPoll() from the UI loop. */
void attakyHapticNotifyPulse();

/** Expire the running pulse; also retries a failed park (throttled). */
void attakyHapticPoll();

#endif
