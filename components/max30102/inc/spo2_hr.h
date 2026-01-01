#ifndef COMPONENTS_SPO2_HR_H
#define COMPONENTS_SPO2_HR_H

#include <cstdint>
#include <array>

namespace muc
{
namespace max30102
{

static constexpr int WINDOW = 100;

class SpO2HrProcessor
{
  public:
    SpO2HrProcessor() noexcept;

    void addSample(std::uint32_t red, std::uint32_t ir);
    bool computeSpO2(float& spo2_out);
    bool computeHeartRate(float& bpm_out);

  private:
    // Raw samples
    std::array<std::uint32_t, WINDOW> m_redBuf;
    std::array<std::uint32_t, WINDOW> m_irBuf;

    // AC components
    std::array<float, WINDOW> m_acRedBuf;
    std::array<float, WINDOW> m_acIrBuf;

    // HR detection buffer
    std::array<float, WINDOW> m_irAcBuf;

    // DC baselines
    float m_dcRed;
    float m_dcIr;

    // State
    int m_index;
    bool m_filled;

    // HR peak detection
    int m_lastPeakIndex;
    float m_lastIrAc;
    float m_peakThreshold;
    bool m_inRefractory;
    int m_sampleCounter;

    // Helpers
    float computeRms(const float* buf, int n) const;
    void updateIrAc();
    bool detectHrPeak(int currentIndex);
};

} // namespace max30102
} // namespace muc

#endif // COMPONENTS_SPO2_HR_H
