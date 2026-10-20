// SPDX-License-Identifier: GPL-3.0-or-later
//
// INA226 current/voltage monitor driver (TI datasheet SBOS547).
//
// Only the bus voltage is exposed: the firmware consumes voltage only (SoC
// is the UI's voltage-based estimate), so no current calibration is
// programmed.
//
// Self-contained: no project debug macros. The caller owns the shared I2C
// bus lock -- this driver performs raw Wire operations and does NOT call
// attakyI2cLock/Unlock itself, to avoid double-locking when invoked from
// AttakyMeshSeriesBoard::getBattMilliVolts() which already holds the lock.

#ifndef INA226_DRIVER_H
#define INA226_DRIVER_H

#include <Arduino.h>
#include <Wire.h>

// INA226 register map (TI datasheet SBOS547). All registers 16-bit,
// big-endian on the bus (MSB first).
#define INA226_REG_CONFIG        0x00
#define INA226_REG_SHUNT_VOLTAGE 0x01
#define INA226_REG_BUS_VOLTAGE   0x02
#define INA226_REG_POWER         0x03
#define INA226_REG_CURRENT       0x04
#define INA226_REG_CALIBRATION   0x05
#define INA226_REG_MANUFACTURER_ID 0xFE
#define INA226_REG_DIE_ID        0xFF

// Reset-default configuration: continuous bus+shunt conversion, 1.1 ms
// conversion time, no averaging. Written explicitly at probe time so prior
// module state cannot leave the chip in power-down mode.
#define INA226_CONFIG_DEFAULT    0x4127

// Identity values (TI datasheet, Manufacturer ID / Die ID registers).
#define INA226_MANUFACTURER_ID_VALUE 0x5449   // "TI"
#define INA226_DIE_ID_VALUE_A        0x2260   // TI SBOS547C Table 7-1
#define INA226_DIE_ID_VALUE_B        0x2261   // also documented there

class INA226Driver {
public:
    // addr: I2C 7-bit address. The 18650 Cell 1.0 strap (A0=A1=GND per the
    // TI address table) selects 0x40.
    INA226Driver(uint8_t addr = INA226_DEFAULT_ADDR);
    ~INA226Driver();

    // Identity-verified probe: reads the DIE_ID register and requires
    // 0x2260 or 0x2261, then programs the default continuous config. Returns true
    // only if the die-ID matched and the config write was acked. A bare address
    // ack cannot separate the INA226 from the INA219 (both span 0x40-0x4F
    // and share the register layout at 0x00-0x05), but the INA219 has no
    // die-ID register -- only a genuine INA226 returns 0x2260/0x2261 here.
    bool begin();

    // Live bus-voltage reading in V. Returns NaN on any I2C read failure
    // -- the failure is propagated via lastReadOK so downstream isnan()
    // checks are not fooled by the 0xFFFF raw sentinel (same discipline as
    // INA219Driver).
    float readBusVoltage();    // V

    // True iff the most recent register read got a clean 2-byte ack.
    bool isLastReadOK() const { return lastReadOK; }

private:
    uint8_t i2cAddr;
    bool lastReadOK;    // most recent readRegister16() clean-ack state

    uint16_t readRegister16(uint8_t reg);
    bool writeRegister16(uint8_t reg, uint16_t value);

public:
    // 18650 Cell 1.0 module strap: A0=A1=GND -> 0b100_0000 = 0x40.
    static constexpr uint8_t INA226_DEFAULT_ADDR = 0x40;
};

#endif
