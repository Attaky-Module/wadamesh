// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <stdbool.h>

// Core front/top buttons on the board's own AW9523 @ 0x59 (P00-P07, all inputs
// with external pull-ups, LOW = pressed). Only POWER_BTN (P07) is consumed today:
// it toggles the panel on/off. The remaining keys are read but not routed, since
// the touch UI has no navigation semantics to map them onto.
//
// POWER_BTN is also wired to the board's hardware power control: a LONG press
// cuts power outside firmware control, so only the short press is ours.

/** Poll the expander. Call once per UI tick; rate-limits its own I2C traffic. */
void attakyKeysPoll();

/** True once per POWER_BTN press edge (consumes the event). */
bool attakyPowerKeyPressed();

/** Whether the @0x59 expander answered at init (false = keys unavailable). */
bool attakyKeysPresent();
