// SPDX-License-Identifier: GPL-3.0-or-later

#include "INA219Driver.h"

INA219Driver::INA219Driver(uint8_t addr, float shuntOhm)
    : i2cAddr(addr), shuntOhm(shuntOhm), currentLSB(0.0f), lastReadOK(false) {
}

INA219Driver::~INA219Driver() {
}

bool INA219Driver::begin() {
    // Probe: a clean ack on the address is the presence signal.
    Wire.beginTransmission(i2cAddr);
    if (Wire.endTransmission() != 0) return false;

    // Soft reset to clear any prior configuration, then program defaults.
    reset();
    delay(2);
    configure();

    // Bit 14 is unused and reads 0, so it is masked before the compare.
    // The INA226 is probed first by die-ID, so the echo mainly rejects an
    // unrelated device that acks the address.
    uint16_t echo = readRegister16(INA219_REG_CONFIG);
    return lastReadOK && (echo | 0x4000) == INA219_CONFIG_DEFAULT;
}

void INA219Driver::reset() {
    writeRegister16(INA219_REG_CONFIG, INA219_CONFIG_RESET);
    delay(2);
}

void INA219Driver::configure() {
    // 1 mA current LSB -> the 16-bit current register covers
    // -32.768 A .. +32.767 A, well over a small Li-ion pack's expected
    // charge/discharge range. CAL register is 15-bit (datasheet eq. 3).
    currentLSB = 0.001f;

    float cal = 0.04096f / (currentLSB * shuntOhm);
    if (cal > 32767.0f) cal = 32767.0f;
    if (cal < 1.0f)     cal = 1.0f;
    uint16_t calReg = (uint16_t)truncf(cal);

    writeRegister16(INA219_REG_CONFIG, INA219_CONFIG_DEFAULT);
    writeRegister16(INA219_REG_CALIBRATION, calReg);

    // Re-derive the actual current LSB from the truncated cal value so
    // readCurrent()/readPower() match what the chip computes, not what we
    // asked for.
    currentLSB = 0.04096f / ((float)calReg * shuntOhm);
}

float INA219Driver::readBusVoltage() {
    uint16_t raw = readRegister16(INA219_REG_BUS_VOLTAGE);
    if (!lastReadOK) return (float)NAN;
    // Bits [15:3] = 13-bit unsigned BUS_VOLTAGE (LSB 4 mV).
    // Bit 2 reserved, bit 1 = CNVR, bit 0 = OVF. Right-shift by 3 to recover
    // the field before applying the LSB -- masking without the shift
    // inflates the reading 8x (a real 4.18 V shows as 33.47 V).
    uint16_t v = (raw & 0xFFF8) >> 3;
    return (float)v * 0.004f;  // 4 mV/bit
}

float INA219Driver::readShuntVoltage() {
    uint16_t raw = readRegister16(INA219_REG_SHUNT_VOLTAGE);
    if (!lastReadOK) return (float)NAN;
    int16_t sv = (int16_t)raw;
    return (float)sv * 0.00001f;  // 10 uV/bit signed
}

float INA219Driver::readCurrent() {
    uint16_t raw = readRegister16(INA219_REG_CURRENT);
    if (!lastReadOK) return (float)NAN;
    int16_t sv = (int16_t)raw;
    return (float)sv * currentLSB;
}

float INA219Driver::readPower() {
    uint16_t raw = readRegister16(INA219_REG_POWER);
    if (!lastReadOK) return (float)NAN;
    int16_t sv = (int16_t)raw;
    // Power LSB = 20 * current_LSB (datasheet eq. 4).
    return (float)sv * (20.0f * currentLSB);
}

uint16_t INA219Driver::readRegister16(uint8_t reg) {
    // Pessimistic default: a read that never reaches the success path
    // leaves lastReadOK == false so the typed readers propagate NaN instead
    // of trusting the 0xFFFF sentinel (which would compute into small but
    // finite non-NaN values and fool downstream isnan() checks).
    lastReadOK = false;
    Wire.beginTransmission(i2cAddr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return 0xFFFF;

    Wire.requestFrom(i2cAddr, (uint8_t)2);
    if (Wire.available() < 2) return 0xFFFF;
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();
    lastReadOK = true;
    return ((uint16_t)msb << 8) | lsb;
}

void INA219Driver::writeRegister16(uint8_t reg, uint16_t value) {
    Wire.beginTransmission(i2cAddr);
    Wire.write(reg);
    Wire.write((uint8_t)(value >> 8));
    Wire.write((uint8_t)(value & 0xFF));
    Wire.endTransmission();
}
