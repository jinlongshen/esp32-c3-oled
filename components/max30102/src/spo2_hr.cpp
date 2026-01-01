#include "spo2_hr.h"
#include <cmath>

namespace muc
{
namespace max30102
{

// Sampling configuration
static constexpr float kSampleRateHz = 50.0f; // matches vTaskDelay(20 ms)
static constexpr float kRefractorySec = 0.3f; // 300 ms
static constexpr int kRefractorySamples = static_cast<int>(kRefractorySec * kSampleRateHz);

// DC smoothing factor
static constexpr float kDcAlpha = 0.95f; // low-pass filter for DC

SpO2HrProcessor::SpO2HrProcessor() noexcept
: m_redBuf{}
, m_irBuf{}
, m_acRedBuf{}
, m_acIrBuf{}
, m_irAcBuf{}
, m_dcRed(0.0f)
, m_dcIr(0.0f)
, m_index(0)
, m_filled(false)
, m_lastPeakIndex(-1)
, m_lastIrAc(0.0f)
, m_peakThreshold(0.0f)
, m_inRefractory(false)
, m_sampleCounter(0)
{
}

void SpO2HrProcessor::addSample(std::uint32_t red, std::uint32_t ir)
{
    // Initialize DC on first sample
    if (m_sampleCounter == 0)
    {
        m_dcRed = static_cast<float>(red);
        m_dcIr = static_cast<float>(ir);
    }

    // Update DC (low-pass)
    m_dcRed = kDcAlpha * m_dcRed + (1.0f - kDcAlpha) * static_cast<float>(red);
    m_dcIr = kDcAlpha * m_dcIr + (1.0f - kDcAlpha) * static_cast<float>(ir);

    // Compute AC sample
    float acRedSample = static_cast<float>(red) - m_dcRed;
    float acIrSample = static_cast<float>(ir) - m_dcIr;

    // Store raw and AC samples
    m_redBuf[m_index] = red;
    m_irBuf[m_index] = ir;
    m_acRedBuf[m_index] = acRedSample;
    m_acIrBuf[m_index] = acIrSample;

    m_index++;
    if (m_index >= WINDOW)
    {
        m_index = 0;
        m_filled = true;
    }

    m_sampleCounter++;

    if (m_filled)
    {
        updateIrAc();
    }
}

float SpO2HrProcessor::computeRms(const float* buf, int n) const
{
    float sum = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        sum += buf[i] * buf[i];
    }
    return std::sqrt(sum / n);
}

bool SpO2HrProcessor::computeSpO2(float& spo2_out)
{
    if (!m_filled)
    {
        return false;
    }

    // AC from RMS
    float acRed = computeRms(m_acRedBuf.data(), WINDOW);
    float acIr = computeRms(m_acIrBuf.data(), WINDOW);

    // DC from smoothed values
    float dcRedLocal = m_dcRed;
    float dcIrLocal = m_dcIr;

    if (dcRedLocal < 1.0f || dcIrLocal < 1.0f)
    {
        return false;
    }

    float relAcRed = acRed / dcRedLocal;
    float relAcIr = acIr / dcIrLocal;

    // Finger detection
    if (relAcRed < 0.02f || relAcIr < 0.02f)
    {
        return false;
    }

    float r = relAcRed / relAcIr;

    if (r < 0.2f || r > 1.8f)
    {
        return false;
    }

    // Better polynomial (still generic)
    float spo2 = -45.06f * r * r + 30.354f * r + 94.845f;

    if (spo2 < 70.0f || spo2 > 100.0f)
    {
        return false;
    }

    spo2_out = spo2;
    return true;
}

void SpO2HrProcessor::updateIrAc()
{
    // AC threshold for HR peak detection
    float acIrRms = computeRms(m_acIrBuf.data(), WINDOW);
    float dcIrLocal = m_dcIr;

    if (dcIrLocal < 1.0f)
    {
        dcIrLocal = 1.0f;
    }

    m_peakThreshold = (acIrRms / dcIrLocal) * 0.5f;

    if (m_peakThreshold < 0.001f)
    {
        m_peakThreshold = 0.001f;
    }
}

bool SpO2HrProcessor::detectHrPeak(int currentIndex)
{
    float current = m_acIrBuf[currentIndex] / m_dcIr;

    if (m_inRefractory)
    {
        if (m_sampleCounter - m_lastPeakIndex > kRefractorySamples)
        {
            m_inRefractory = false;
        }
        else
        {
            return false;
        }
    }

    bool risingCross = (m_lastIrAc < m_peakThreshold) && (current >= m_peakThreshold);

    m_lastIrAc = current;

    if (!risingCross)
    {
        return false;
    }

    m_lastPeakIndex = m_sampleCounter;
    m_inRefractory = true;
    return true;
}

bool SpO2HrProcessor::computeHeartRate(float& bpm_out)
{
    if (!m_filled)
    {
        return false;
    }

    int currentIndex = m_index - 1;
    if (currentIndex < 0)
    {
        currentIndex = WINDOW - 1;
    }

    if (!detectHrPeak(currentIndex))
    {
        return false;
    }

    static int prevPeakSample = -1;

    if (prevPeakSample < 0)
    {
        prevPeakSample = m_lastPeakIndex;
        return false;
    }

    int deltaSamples = m_lastPeakIndex - prevPeakSample;
    prevPeakSample = m_lastPeakIndex;

    if (deltaSamples <= 0)
    {
        return false;
    }

    float periodSec = static_cast<float>(deltaSamples) / kSampleRateHz;

    if (periodSec <= 0.3f || periodSec >= 2.0f)
    {
        return false;
    }

    float bpm = 60.0f / periodSec;

    if (bpm < 30.0f || bpm > 220.0f)
    {
        return false;
    }

    bpm_out = bpm;
    return true;
}

} // namespace max30102
} // namespace muc
