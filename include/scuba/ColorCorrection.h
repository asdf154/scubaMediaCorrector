#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace scuba {

struct RgbPixel {
    std::uint8_t r{};
    std::uint8_t g{};
    std::uint8_t b{};
};

struct Image {
    int width{};
    int height{};
    std::vector<RgbPixel> pixels;

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
};

struct ManualAdjustments {
    double temperature = 0.0;   // -1.0 cool to +1.0 warm
    double tint = 0.0;          // -1.0 green to +1.0 magenta
    double highlights = 0.0;    // -1.0 to +1.0
    double shadows = 0.0;       // -1.0 to +1.0
    double whites = 0.0;        // -1.0 to +1.0
    double blacks = 0.0;        // -1.0 to +1.0
    double saturation = 0.0;    // -1.0 to +1.0
    double vibrance = 0.0;      // -1.0 to +1.0
    double aquaReduction = 0.35;// 0.0 to 1.0, attenuates cyan/aqua dominance
    double clarity = 0.0;       // 0.0 to 1.0 local contrast approximation
    double sharpness = 0.0;     // 0.0 to 1.0 unsharp-mask strength
};

struct CorrectionSettings {
    double intensity = 1.0;     // 0.0 original to 1.0 full automatic correction
    ManualAdjustments manual{};
};

class ColorCorrectionEngine {
public:
    [[nodiscard]] Image autoCorrect(const Image& input, const CorrectionSettings& settings) const;
    [[nodiscard]] Image splitComparison(const Image& before, const Image& after, double divider = 0.5) const;

private:
    [[nodiscard]] Image runAutomaticPass(const Image& input, const ManualAdjustments& manual) const;
    [[nodiscard]] Image applyClarityAndSharpness(const Image& input, double clarity, double sharpness) const;
};

[[nodiscard]] Image readPpm(const std::string& path);
void writePpm(const std::string& path, const Image& image);

} // namespace scuba
