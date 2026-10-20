// SPDX-License-Identifier: GPL-3.0-or-later

#include "INA226Driver.h"

INA226Driver::INA226Driver(uint8_t addr)
    : i2cAddr(addr), lastReadOK(false) {
}

INA226Driver::~INA226Driver() {
}

bool INA226Driver::begin() {
    // Die ID is the definitive identity check: only an INA226 answers at
    // register 0xFF with 0x2260 or 0x2261 (TI
    // SBOS547C Table 7-1). This must run BEFORE any INA219-style
    // probe on the shared bus -- the INA219 has no die-ID register and its
    // config echo alone cannot distinguish the two chips.
    uint16_t dieId = readRegister16(INA226_REG_DIE_ID);
    if (!lastReadOK || (dieId != INA226_DIE_ID_VALUE_A && dieId != INA226_DIE_ID_VALUE_B)) return false;

    // Reset-default continuous conversion config, written explicitly so
    // prior module state cannot leave the chip in power-down mode.
    return writeRegister16(INA226_REG_CONFIG, INA226_CONFIG_DEFAULT);
}

float INA226Driver::readBusVoltage() {
    uint16_t raw = readRegister16(INA226_REG_BUS_VOLTAGE);
    if (!lastReadOK) return (float)NAN;
    // Unshifted 15-bit voltage field (D15 always 0), LSB 1.25 mV. Unlike
    // the INA219 there is no [15:3] packing -- do not shift or mask it.
    return (float)raw * 0.00125f;
}

uint16_t INA226Driver::readRegister16(uint8_t reg) {
    // Pessimistic default: a read that never reaches the success path
    // leaves lastReadOK == false so typed readers propagate NaN instead of
    // trusting the 0xFFFF sentinel.
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

bool INA226Driver::writeRegister16(uint8_t reg, uint16_t value) {
    Wire.beginTransmission(i2cAddr);
    Wire.write(reg);
    Wire.write((uint8_t)(value >> 8));
    Wire.write((uint8_t)(value & 0xFF));
    return Wire.endTransmission() == 0;
}
