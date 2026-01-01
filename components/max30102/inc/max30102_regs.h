#ifndef COMPONENTS_MAX30102_REGS_H
#define COMPONENTS_MAX30102_REGS_H

#pragma once

#include <cstdint>

namespace muc
{
namespace max30102
{
// -----------------------------------------------------------------------------
// I2C addressing and device identification
// -----------------------------------------------------------------------------

// 7-bit I2C address (datasheet uses 0xAE/0xAF as 8-bit R/W variants of this)
constexpr std::uint8_t I2C_ADDRESS = 0x57; // 0b1010111

// Expected PART ID value (read from REG::PartId)
constexpr std::uint8_t PART_ID_EXPECTED = 0x15; // MAX30102 device ID

// Optional: Revision ID register value is implementation-specific; no fixed constant here.

// -----------------------------------------------------------------------------
// Register map (addresses from datasheet register table)
// -----------------------------------------------------------------------------

enum class Register : std::uint8_t
{
    // Interrupt status and enable
    IntStatus1 = 0x00, // Interrupt Status 1
    IntStatus2 = 0x01, // Interrupt Status 2
    IntEnable1 = 0x02, // Interrupt Enable 1
    IntEnable2 = 0x03, // Interrupt Enable 2

    // FIFO pointers and data
    FifoWritePtr = 0x04, // FIFO Write Pointer
    FifoOverflow = 0x05, // FIFO Overflow Counter
    FifoReadPtr = 0x06,  // FIFO Read Pointer
    FifoData = 0x07,     // FIFO Data Register

    // FIFO and mode configuration
    FifoConfig = 0x08, // FIFO Configuration
    ModeConfig = 0x09, // Mode Configuration (shutdown, reset, mode select)
    Spo2Config = 0x0A, // SpO2 Configuration (ADC range, sample rate, pulse width)

    // LED pulse amplitudes
    Led1PulseAmp = 0x0C, // LED1 (RED) Pulse Amplitude
    Led2PulseAmp = 0x0D, // LED2 (IR) Pulse Amplitude

    // Multi-LED control
    MultiLed1 = 0x11, // Multi-LED Mode Control Register 1 (Slots 1–2)
    MultiLed2 =
        0x12, // Multi-LED Mode Control Register 2 (Slots 3–4; unused in MAX30102 but defined)

    // Temperature
    TempInt = 0x1F,    // Die Temperature Integer
    TempFrac = 0x20,   // Die Temperature Fraction
    TempConfig = 0x21, // Die Temperature Configuration

    // Identification
    RevId = 0xFE, // Revision ID (read-only)
    PartId = 0xFF // Part ID (read-only, expected 0x15)
};

// Helper: convert strongly-typed Register to underlying address
constexpr std::uint8_t reg(Register r) noexcept
{
    return static_cast<std::uint8_t>(r);
}

// -----------------------------------------------------------------------------
// Interrupt status / enable bitfields
// -----------------------------------------------------------------------------
//
// INT_STATUS_1 / INT_ENABLE_1 (0x00 / 0x02)
//  bit7: A_FULL      – FIFO almost full
//  bit6: PPG_RDY     – new FIFO data ready
//  bit5: ALC_OVF     – ambient light cancellation overflow
//  bit4: PROX_INT    – proximity threshold triggered
//  bits3:0 reserved
//
// INT_STATUS_2 / INT_ENABLE_2 (0x01 / 0x03)
//  bit1: DIE_TEMP_RDY – temperature conversion ready
//  others reserved
// -----------------------------------------------------------------------------

namespace Int1Bits
{
constexpr std::uint8_t A_FULL = 1u << 7;   // FIFO Almost Full
constexpr std::uint8_t PPG_RDY = 1u << 6;  // New FIFO data ready
constexpr std::uint8_t ALC_OVF = 1u << 5;  // Ambient light cancellation overflow
constexpr std::uint8_t PROX_INT = 1u << 4; // Proximity interrupt
constexpr std::uint8_t ALL_MASK = A_FULL | PPG_RDY | ALC_OVF | PROX_INT;
} // namespace Int1Bits

namespace Int2Bits
{
constexpr std::uint8_t DIE_TEMP_RDY = 1u << 1; // Die temperature conversion ready
constexpr std::uint8_t ALL_MASK = DIE_TEMP_RDY;
} // namespace Int2Bits

// -----------------------------------------------------------------------------
// FIFO configuration (REG_FIFO_CONFIG = 0x08)
// -----------------------------------------------------------------------------
//
//  bits7:5 SMP_AVE[2:0]   – sample averaging (1, 2, 4, 8, 16, 32 samples)
//  bit4    FIFO_ROLLOVER_EN – 1: enable FIFO rollover
//  bits3:0 FIFO_A_FULL[3:0] – FIFO almost full threshold (#samples)
//
// SMP_AVE encodings (datasheet table):
//  000: 1 sample
//  001: 2 samples
//  010: 4 samples
//  011: 8 samples
//  100: 16 samples
//  101: 32 samples
// -----------------------------------------------------------------------------

namespace FifoConfigBits
{
constexpr std::uint8_t SMP_AVE_MASK = 0b1110'0000u; // bits 7:5
constexpr std::uint8_t SMP_AVE_SHIFT = 5;

constexpr std::uint8_t ROLLOVER_EN = 1u << 4; // FIFO rollover enable

constexpr std::uint8_t A_FULL_MASK = 0b0000'1111u; // bits 3:0

// Sample averaging encodings (value << SMP_AVE_SHIFT)
constexpr std::uint8_t SMP_AVE_1 = 0b000u << SMP_AVE_SHIFT;
constexpr std::uint8_t SMP_AVE_2 = 0b001u << SMP_AVE_SHIFT;
constexpr std::uint8_t SMP_AVE_4 = 0b010u << SMP_AVE_SHIFT;
constexpr std::uint8_t SMP_AVE_8 = 0b011u << SMP_AVE_SHIFT;
constexpr std::uint8_t SMP_AVE_16 = 0b100u << SMP_AVE_SHIFT;
constexpr std::uint8_t SMP_AVE_32 = 0b101u << SMP_AVE_SHIFT;
} // namespace FifoConfigBits

// -----------------------------------------------------------------------------
// Mode configuration (REG_MODE_CONFIG = 0x09)
// -----------------------------------------------------------------------------
//
//  bit7    SHDN       – shutdown (1 = low-power shutdown)
//  bit6    RESET      – reset device (self-clearing)
//  bits2:0 MODE[2:0]  – operating mode
//
// MODE encodings (datasheet):
//  010: HR (red only)
//  011: SpO2 (red + IR)
//  (other modes are for devices with more LEDs; only these two are relevant)
// -----------------------------------------------------------------------------

namespace ModeConfigBits
{
constexpr std::uint8_t SHDN = 1u << 7;           // Shutdown
constexpr std::uint8_t RESET = 1u << 6;          // Reset
constexpr std::uint8_t MODE_MASK = 0b0000'0111u; // bits 2:0

// Mode encodings
constexpr std::uint8_t MODE_HR_RED_ONLY = 0b010u; // red LED only
constexpr std::uint8_t MODE_SPO2_RED_IR = 0b011u; // red + IR LEDs
} // namespace ModeConfigBits

// -----------------------------------------------------------------------------
// SpO2 configuration (REG_SPO2_CONFIG = 0x0A)
// -----------------------------------------------------------------------------
//
//  bits6:5 SPO2_ADC_RGE[1:0] – ADC full-scale range
//  bits4:2 SPO2_SR[2:0]      – sample rate
//  bits1:0 LED_PW[1:0]       – LED pulse width (and ADC resolution)
//
// SPO2_ADC_RGE encodings (datasheet):
//  00: 2048 nA
//  01: 4096 nA
//  10: 8192 nA
//  11: 16384 nA
//
// SPO2_SR encodings (examples; see datasheet for full mapping):
//  000: 50 sps
//  001: 100 sps
//  010: 200 sps
//  011: 400 sps
//  100: 800 sps
//  101: 1000 sps
//  110: 1600 sps
//  111: 3200 sps
//
// LED_PW encodings (pulse width, ADC resolution):
//  00: 69  µs, 15-bit
//  01: 118 µs, 16-bit
//  10: 215 µs, 17-bit
//  11: 411 µs, 18-bit
// -----------------------------------------------------------------------------

namespace Spo2ConfigBits
{
// ADC range
constexpr std::uint8_t ADC_RGE_MASK = 0b0110'0000u; // bits 6:5
constexpr std::uint8_t ADC_RGE_SHIFT = 5;

constexpr std::uint8_t ADC_RGE_2048nA = 0b00u << ADC_RGE_SHIFT;
constexpr std::uint8_t ADC_RGE_4096nA = 0b01u << ADC_RGE_SHIFT;
constexpr std::uint8_t ADC_RGE_8192nA = 0b10u << ADC_RGE_SHIFT;
constexpr std::uint8_t ADC_RGE_16384nA = 0b11u << ADC_RGE_SHIFT;

// Sample rate
constexpr std::uint8_t SR_MASK = 0b0001'1100u; // bits 4:2
constexpr std::uint8_t SR_SHIFT = 2;

constexpr std::uint8_t SR_50_SPS = 0b000u << SR_SHIFT;
constexpr std::uint8_t SR_100_SPS = 0b001u << SR_SHIFT;
constexpr std::uint8_t SR_200_SPS = 0b010u << SR_SHIFT;
constexpr std::uint8_t SR_400_SPS = 0b011u << SR_SHIFT;
constexpr std::uint8_t SR_800_SPS = 0b100u << SR_SHIFT;
constexpr std::uint8_t SR_1000_SPS = 0b101u << SR_SHIFT;
constexpr std::uint8_t SR_1600_SPS = 0b110u << SR_SHIFT;
constexpr std::uint8_t SR_3200_SPS = 0b111u << SR_SHIFT;

// Pulse width / resolution
constexpr std::uint8_t LED_PW_MASK = 0b0000'0011u; // bits 1:0

constexpr std::uint8_t LED_PW_69_US_15BIT = 0b00u;
constexpr std::uint8_t LED_PW_118_US_16BIT = 0b01u;
constexpr std::uint8_t LED_PW_215_US_17BIT = 0b10u;
constexpr std::uint8_t LED_PW_411_US_18BIT = 0b11u;
} // namespace Spo2ConfigBits

// -----------------------------------------------------------------------------
// LED pulse amplitude registers (REG_LED1_PULSE_AMP, REG_LED2_PULSE_AMP)
// -----------------------------------------------------------------------------
//
//  8-bit values, 0x00–0xFF, mapping to LED current per datasheet.
//  No bitfields; write raw values.
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Multi-LED mode control (REG_MULTI_LED_1 = 0x11, REG_MULTI_LED_2 = 0x12)
// -----------------------------------------------------------------------------
//
// REG_MULTI_LED_1:
//  bits7:4 SLOT2[3:0]
//  bits3:0 SLOT1[3:0]
//
// REG_MULTI_LED_2:
//  bits7:4 SLOT4[3:0]
//  bits3:0 SLOT3[3:0]
//
// Slot encodings (for MAX30102, only LED1 (red) and LED2 (IR) are meaningful):
//  0000: none
//  0001: RED (LED1)
//  0010: IR  (LED2)
// -----------------------------------------------------------------------------

enum class Slot : std::uint8_t
{
    None = 0x00,
    Red = 0x01, // LED1
    IR = 0x02   // LED2
};

namespace MultiLedBits
{
// masks and shifts for slot fields
constexpr std::uint8_t SLOT1_MASK = 0b0000'1111u;
constexpr std::uint8_t SLOT2_MASK = 0b1111'0000u;
constexpr std::uint8_t SLOT3_MASK = 0b0000'1111u;
constexpr std::uint8_t SLOT4_MASK = 0b1111'0000u;

constexpr std::uint8_t SLOT1_SHIFT = 0;
constexpr std::uint8_t SLOT2_SHIFT = 4;
constexpr std::uint8_t SLOT3_SHIFT = 0;
constexpr std::uint8_t SLOT4_SHIFT = 4;
} // namespace MultiLedBits

// -----------------------------------------------------------------------------
// Temperature configuration (REG_TEMP_CONFIG = 0x21)
// -----------------------------------------------------------------------------
//
//  bit0: TEMP_EN – write 1 to start a temperature conversion.
//  Bit self-clears when conversion is done. DIE_TEMP_RDY interrupt fires when ready.
// -----------------------------------------------------------------------------

namespace TempConfigBits
{
constexpr std::uint8_t TEMP_EN = 1u << 0;
}

// -----------------------------------------------------------------------------
// Utility: LED mode enumeration (matches MODE field in ModeConfig)
// -----------------------------------------------------------------------------

enum class LedMode : std::uint8_t
{
    HrRedOnly = ModeConfigBits::MODE_HR_RED_ONLY,
    Spo2RedIr = ModeConfigBits::MODE_SPO2_RED_IR
};

// -----------------------------------------------------------------------------
// Utility: FIFO constants
// -----------------------------------------------------------------------------

// FIFO depth in samples (18-bit samples, stored as 3 bytes each per LED)
constexpr std::uint8_t FIFO_CAPACITY_SAMPLES = 32; // 5-bit pointers, 0–31

} // namespace max30102
} // namespace muc

#endif // COMPONENTS_MAX30102_REGS_H