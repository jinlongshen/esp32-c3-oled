#ifndef COMPONENTS_MAX30102_REGS_H
#define COMPONENTS_MAX30102_REGS_H

#pragma once

#include <cstdint>

namespace muc
{
namespace max30102
{

// -----------------------------------------------------------------------------
// I2C Address
// -----------------------------------------------------------------------------
constexpr std::uint8_t I2C_ADDRESS = 0x57;

// -----------------------------------------------------------------------------
// Register Map (MAX30102 datasheet)
// -----------------------------------------------------------------------------
enum class Register : std::uint8_t
{
    IntStatus1 = 0x00,
    IntStatus2 = 0x01,
    IntEnable1 = 0x02,
    IntEnable2 = 0x03,

    FifoWritePtr = 0x04,
    FifoOverflow = 0x05,
    FifoReadPtr = 0x06,
    FifoData = 0x07,

    FifoConfig = 0x08,
    ModeConfig = 0x09,
    Spo2Config = 0x0A,

    Led1PulseAmp = 0x0C, // RED
    Led2PulseAmp = 0x0D, // IR

    MultiLed1 = 0x11,
    MultiLed2 = 0x12,

    TempInt = 0x1F,
    TempFrac = 0x20,
    TempConfig = 0x21,

    RevId = 0xFE,
    PartId = 0xFF
};

// Helper
constexpr std::uint8_t reg(Register r) noexcept
{
    return static_cast<std::uint8_t>(r);
}

// -----------------------------------------------------------------------------
// Interrupt Bits
// -----------------------------------------------------------------------------
namespace Int1Bits
{
constexpr std::uint8_t A_FULL = 1u << 7;
constexpr std::uint8_t PPG_RDY = 1u << 6;
constexpr std::uint8_t ALC_OVF = 1u << 5;
constexpr std::uint8_t PROX_INT = 1u << 4;
} // namespace Int1Bits

namespace Int2Bits
{
constexpr std::uint8_t DIE_TEMP_RDY = 1u << 1;
}

// -----------------------------------------------------------------------------
// FIFO Configuration (0x08)
// -----------------------------------------------------------------------------
namespace FifoConfigBits
{
constexpr std::uint8_t SMP_AVE_MASK = 0b11100000;
constexpr std::uint8_t SMP_AVE_SHIFT = 5;

constexpr std::uint8_t ROLLOVER_EN = 1u << 4;

constexpr std::uint8_t A_FULL_MASK = 0b00001111;

// Encodings
constexpr std::uint8_t SMP_AVE_1 = 0b000 << 5;
constexpr std::uint8_t SMP_AVE_2 = 0b001 << 5;
constexpr std::uint8_t SMP_AVE_4 = 0b010 << 5;
constexpr std::uint8_t SMP_AVE_8 = 0b011 << 5;
constexpr std::uint8_t SMP_AVE_16 = 0b100 << 5;
constexpr std::uint8_t SMP_AVE_32 = 0b101 << 5;
} // namespace FifoConfigBits

// -----------------------------------------------------------------------------
// Mode Configuration (0x09)
// -----------------------------------------------------------------------------
namespace ModeConfigBits
{
constexpr std::uint8_t SHDN = 1u << 7;
constexpr std::uint8_t RESET = 1u << 6;
constexpr std::uint8_t MODE_MASK = 0b00000111;

constexpr std::uint8_t MODE_HR_RED_ONLY = 0b010;
constexpr std::uint8_t MODE_SPO2_RED_IR = 0b011;
} // namespace ModeConfigBits

enum class LedMode : std::uint8_t
{
    HrRedOnly = ModeConfigBits::MODE_HR_RED_ONLY,
    Spo2RedIr = ModeConfigBits::MODE_SPO2_RED_IR
};

// -----------------------------------------------------------------------------
// SpO2 Configuration (0x0A)
// -----------------------------------------------------------------------------
namespace Spo2ConfigBits
{

// ADC Range
constexpr std::uint8_t ADC_RGE_MASK = 0b01100000;
constexpr std::uint8_t ADC_RGE_SHIFT = 5;

constexpr std::uint8_t ADC_RGE_2048 = 0b00 << 5;
constexpr std::uint8_t ADC_RGE_4096 = 0b01 << 5;
constexpr std::uint8_t ADC_RGE_8192 = 0b10 << 5;
constexpr std::uint8_t ADC_RGE_16384 = 0b11 << 5;

// Sample Rate
constexpr std::uint8_t SR_MASK = 0b00011100;
constexpr std::uint8_t SR_SHIFT = 2;

constexpr std::uint8_t SR_50 = 0b000 << 2;
constexpr std::uint8_t SR_100 = 0b001 << 2;
constexpr std::uint8_t SR_200 = 0b010 << 2;
constexpr std::uint8_t SR_400 = 0b011 << 2;
constexpr std::uint8_t SR_800 = 0b100 << 2;
constexpr std::uint8_t SR_1000 = 0b101 << 2;
constexpr std::uint8_t SR_1600 = 0b110 << 2;
constexpr std::uint8_t SR_3200 = 0b111 << 2;

// Pulse Width
constexpr std::uint8_t LED_PW_MASK = 0b00000011;

constexpr std::uint8_t PW_69 = 0b00;
constexpr std::uint8_t PW_118 = 0b01;
constexpr std::uint8_t PW_215 = 0b10;
constexpr std::uint8_t PW_411 = 0b11;
} // namespace Spo2ConfigBits

// Strongly typed enums for your driver
enum class AdcRange : std::uint8_t
{
    Range2048 = 0b00,
    Range4096 = 0b01,
    Range8192 = 0b10,
    Range16384 = 0b11
};

enum class SampleRate : std::uint8_t
{
    Rate50 = 0b000,
    Rate100 = 0b001,
    Rate200 = 0b010,
    Rate400 = 0b011,
    Rate800 = 0b100,
    Rate1000 = 0b101,
    Rate1600 = 0b110,
    Rate3200 = 0b111
};

enum class PulseWidth : std::uint8_t
{
    Us69 = 0b00,
    Us118 = 0b01,
    Us215 = 0b10,
    Us411 = 0b11
};

// -----------------------------------------------------------------------------
// Multi-LED Slots (0x11, 0x12)
// -----------------------------------------------------------------------------
enum class Slot : std::uint8_t
{
    None = 0x00,
    Red = 0x01,
    IR = 0x02
};

namespace MultiLedBits
{
constexpr std::uint8_t SLOT1_SHIFT = 0;
constexpr std::uint8_t SLOT2_SHIFT = 4;
constexpr std::uint8_t SLOT3_SHIFT = 0;
constexpr std::uint8_t SLOT4_SHIFT = 4;

constexpr std::uint8_t SLOT_MASK = 0x0F;
} // namespace MultiLedBits

// -----------------------------------------------------------------------------
// Temperature Config (0x21)
// -----------------------------------------------------------------------------
namespace TempConfigBits
{
constexpr std::uint8_t TEMP_EN = 1u << 0;
}

} // namespace max30102
} // namespace muc

#endif // COMPONENTS_MAX30102_REGS_H
