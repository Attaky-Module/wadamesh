// SPDX-License-Identifier: GPL-3.0-or-later
//
// INA219 current/voltage monitor driver (TI datasheet SBOS448).
//
// Self-contained: no project debug macros. The caller owns the shared I2C
// bus lock -- this driver performs raw Wire operations and does NOT call
// attakyI2cLock/Unlock itself, to avoid double-locking when invoked from
// AttakyMeshSeriesBoard::getBattMilliVolts() which already holds the lock.

#ifndef INA219_DRIVER_H
#define INA219_DRIVER_H

#include <Arduino.h>
#include <Wire.h>

// INA219 register map (TI datasheet SBOS448).
// All registers are 16-bit, big-endian on the bus (MSB first).
#define INA219_REG_CONFIG        0x00
#define INA219_REG_SHUNT_VOLTAGE 0x01
#define INA219_REG_BUS_VOLTAGE   0x02
#define INA219_REG_POWER         0x03
#define INA219_REG_CURRENT       0x04
#define INA219_REG_CALIBRATION   0x05

// CONFIG register bits, as validated on hardware:
//   RST=0, bit14=1 (reserved), BRNG=0 (16V FSR), PG=0b00 (+/-40mV shunt FSR),
//   BADC=0b0011 (12-bit, 532us), SADC=0b0011 (12-bit, 532us),
//   MODE=0b111 (shunt+bus continuous). The datasheet reset value is 0x399F.
#define INA219_CONFIG_RESET      0x8000
#define INA219_CONFIG_DEFAULT    0x419F

// Bus voltage LSB = 4 mV/bit (BRNG=0, 16V FSR). Shunt voltage LSB = 10 uV/bit.
// Current LSB depends on calibration; CAL is computed from the desired
// current_LSB and the shunt resistance (datasheet eq. 3):
//   CAL = trunc(0.04096 / (current_LSB * R_shunt_ohm))
// Power LSB = 20 * current_LSB (datasheet eq. 4).

class INA219Driver {
public:
    // addr: I2C 7-bit address (0x40-0x4F). shuntOhm: shunt resistance in ohm.
    INA219Driver(uint8_t addr = INA219_DEFAULT_ADDR, float shuntOhm = INA219_DEFAULT_SHUNT_OHM);
    ~INA219Driver();

    // Soft-reset + configure + calibrate, then verify by reading CONFIG
    // back (bit 14 always reads 0). Returns true only if the chip acked its
    // address and echoed the written config.
    bool begin();
    void reset();

    // Live readings (units: V, V, A, W). Return NaN on any I2C read failure
    // -- the failure is propagated via lastReadOK so downstream isnan()
    // checks are not fooled by the 0xFFFF raw sentinel computing into small
    // but finite (and thus non-NaN) values.
    float readBusVoltage();     // V
    float readShuntVoltage();   // V (signed shunt drop)
    float readCurrent();        // A (signed; calibration-derived LSB)
    float readPower();          // W (calibration-derived LSB)

    // True iff the most recent register read got a clean 2-byte ack.
    bool isLastReadOK() const { return lastReadOK; }

private:
    uint8_t i2cAddr;
    float shuntOhm;
    float currentLSB;   // A/bit, re-derived in configure() from truncated CAL
    bool lastReadOK;    // most recent readRegister16() clean-ack state

    uint16_t readRegister16(uint8_t reg);
    void writeRegister16(uint8_t reg, uint16_t value);
    void configure();

public:
    // Power Standard Cell 1.9 module strap (from ina.pdf schematic):
    // A1 pulled to VS (HIGH) via R11, A0 pulled to GND (LOW) via R9
    // -> 7-bit address 0b100_0100 = 0x44. Shunt R12 = 100 mOhm.
    static constexpr uint8_t INA219_DEFAULT_ADDR      = 0x44;
    static constexpr float   INA219_DEFAULT_SHUNT_OHM = 0.1f;
};

#endif
