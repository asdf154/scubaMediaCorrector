#include "scuba/ColorCorrection.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace scuba {
namespace {

constexpr double kEpsilon = 1e-6;

[[nodiscard]] double clamp01(double value) {
    return std::clamp(value, 0.0, 1.0);
}

[[nodiscard]] double clampUnit(double value) {
    return std::clamp(value, -1.0, 1.0);
}

[[nodiscard]] std::uint8_t toByte(double value) {
    return static_cast<std::uint8_t>(std::lround(std::clamp(value, 0.0, 255.0)));
}

[[nodiscard]] RgbPixel lerp(const RgbPixel& a, const RgbPixel& b, double amount) {
    const double t = clamp01(amount);
    return {
        toByte(a.r + (static_cast<double>(b.r) - a.r) * t),
        toByte(a.g + (static_cast<double>(b.g) - a.g) * t),
        toByte(a.b + (static_cast<double>(b.b) - a.b) * t),
    };
}

[[nodiscard]] double luminance(const RgbPixel& pixel) {
    return (0.2126 * pixel.r + 0.7152 * pixel.g + 0.0722 * pixel.b) / 255.0;
}

[[nodiscard]] RgbPixel adjustSaturation(const RgbPixel& pixel, double saturation, double vibrance) {
    const double gray = 0.299 * pixel.r + 0.587 * pixel.g + 0.114 * pixel.b;
    const double maxChannel = std::max({pixel.r, pixel.g, pixel.b}) / 255.0;
    const double minChannel = std::min({pixel.r, pixel.g, pixel.b}) / 255.0;
    const double colorfulness = maxChannel - minChannel;
    const double satScale = 1.0 + 0.8 * clampUnit(saturation);
    const double vibScale = 1.0 + 0.9 * clampUnit(vibrance) * (1.0 - colorfulness);
    const double scale = std::max(0.0, satScale * vibScale);

    return {
        toByte(gray + (pixel.r - gray) * scale),
        toByte(gray + (pixel.g - gray) * scale),
        toByte(gray + (pixel.b - gray) * scale),
    };
}

[[nodiscard]] RgbPixel adjustTone(const RgbPixel& pixel, const ManualAdjustments& manual) {
    const double lum = luminance(pixel);
    const double shadowMask = std::pow(1.0 - lum, 1.8);
    const double highlightMask = std::pow(lum, 1.8);
    const double midMask = 1.0 - std::abs(lum - 0.5) * 2.0;

    const double exposureOffset =
        44.0 * clampUnit(manual.shadows) * shadowMask +
        44.0 * clampUnit(manual.highlights) * highlightMask +
        32.0 * clampUnit(manual.whites) * std::pow(lum, 3.0) +
        32.0 * clampUnit(manual.blacks) * std::pow(1.0 - lum, 3.0);

    const double contrast = 1.0 + 0.12 * midMask;
    return {
        toByte((pixel.r - 128.0) * contrast + 128.0 + exposureOffset),
        toByte((pixel.g - 128.0) * contrast + 128.0 + exposureOffset),
        toByte((pixel.b - 128.0) * contrast + 128.0 + exposureOffset),
    };
}

[[nodiscard]] RgbPixel reduceAquaDominance(const RgbPixel& pixel, double amount) {
    const double cyanDominance = clamp01((static_cast<double>(pixel.g) + pixel.b - 2.0 * pixel.r) / 255.0);
    const double reduction = clamp01(amount) * cyanDominance;
    return {
        toByte(pixel.r + 42.0 * reduction),
        toByte(pixel.g * (1.0 - 0.18 * reduction)),
        toByte(pixel.b * (1.0 - 0.26 * reduction)),
    };
}

[[nodiscard]] RgbPixel applyTemperatureTint(const RgbPixel& pixel, double temperature, double tint) {
    const double temp = clampUnit(temperature);
    const double magenta = clampUnit(tint);
    return {
        toByte(pixel.r + 24.0 * temp + 10.0 * magenta),
        toByte(pixel.g - 8.0 * temp - 18.0 * magenta),
        toByte(pixel.b - 22.0 * temp + 10.0 * magenta),
    };
}

void validateImage(const Image& image) {
    if (image.width <= 0 || image.height <= 0) {
        throw std::invalid_argument("Image dimensions must be positive");
    }
    if (image.pixels.size() != static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height)) {
        throw std::invalid_argument("Image pixel count does not match dimensions");
    }
}

[[nodiscard]] std::string readToken(std::istream& input) {
    std::string token;
    while (input >> token) {
        if (!token.empty() && token[0] == '#') {
            input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        return token;
    }
    throw std::runtime_error("Unexpected end of PPM header");
}

} // namespace

bool Image::empty() const noexcept {
    return width <= 0 || height <= 0 || pixels.empty();
}

std::size_t Image::size() const noexcept {
    return pixels.size();
}

Image ColorCorrectionEngine::autoCorrect(const Image& input, const CorrectionSettings& settings) const {
    validateImage(input);
    const Image corrected = runAutomaticPass(input, settings.manual);

    Image blended = input;
    const double intensity = clamp01(settings.intensity);
    for (std::size_t i = 0; i < input.pixels.size(); ++i) {
        blended.pixels[i] = lerp(input.pixels[i], corrected.pixels[i], intensity);
    }

    return applyClarityAndSharpness(blended, settings.manual.clarity, settings.manual.sharpness);
}

Image ColorCorrectionEngine::splitComparison(const Image& before, const Image& after, double divider) const {
    validateImage(before);
    validateImage(after);
    if (before.width != after.width || before.height != after.height) {
        throw std::invalid_argument("Before and after images must have matching dimensions");
    }

    Image comparison = before;
    const int splitColumn = static_cast<int>(std::lround(clamp01(divider) * before.width));
    for (int y = 0; y < before.height; ++y) {
        for (int x = splitColumn; x < before.width; ++x) {
            comparison.pixels[static_cast<std::size_t>(y * before.width + x)] = after.pixels[static_cast<std::size_t>(y * before.width + x)];
        }
    }
    return comparison;
}

Image ColorCorrectionEngine::runAutomaticPass(const Image& input, const ManualAdjustments& manual) const {
    validateImage(input);

    double sumR = 0.0;
    double sumG = 0.0;
    double sumB = 0.0;
    for (const auto& pixel : input.pixels) {
        sumR += pixel.r;
        sumG += pixel.g;
        sumB += pixel.b;
    }

    const double count = static_cast<double>(input.pixels.size());
    const double avgR = std::max(sumR / count, kEpsilon);
    const double avgG = std::max(sumG / count, kEpsilon);
    const double avgB = std::max(sumB / count, kEpsilon);
    const double grayTarget = (avgR + avgG + avgB) / 3.0;

    const double redBoost = std::clamp(grayTarget / avgR, 1.0, 2.85);
    const double greenScale = std::clamp(grayTarget / avgG, 0.72, 1.32);
    const double blueScale = std::clamp(grayTarget / avgB, 0.56, 1.18);

    Image output = input;
    for (auto& pixel : output.pixels) {
        RgbPixel balanced{
            toByte(pixel.r * redBoost + 10.0),
            toByte(pixel.g * greenScale),
            toByte(pixel.b * blueScale),
        };
        balanced = reduceAquaDominance(balanced, manual.aquaReduction);
        balanced = applyTemperatureTint(balanced, manual.temperature + 0.16, manual.tint + 0.04);
        balanced = adjustTone(balanced, manual);
        balanced = adjustSaturation(balanced, manual.saturation + 0.18, manual.vibrance + 0.22);
        pixel = balanced;
    }

    return output;
}

Image ColorCorrectionEngine::applyClarityAndSharpness(const Image& input, double clarity, double sharpness) const {
    validateImage(input);
    const double amount = std::max(clamp01(clarity) * 0.45, clamp01(sharpness) * 0.65);
    if (amount <= kEpsilon || input.width < 3 || input.height < 3) {
        return input;
    }

    Image output = input;
    const auto at = [&](int x, int y) -> const RgbPixel& {
        x = std::clamp(x, 0, input.width - 1);
        y = std::clamp(y, 0, input.height - 1);
        return input.pixels[static_cast<std::size_t>(y * input.width + x)];
    };

    for (int y = 0; y < input.height; ++y) {
        for (int x = 0; x < input.width; ++x) {
            double blurR = 0.0;
            double blurG = 0.0;
            double blurB = 0.0;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    const auto& sample = at(x + dx, y + dy);
                    blurR += sample.r;
                    blurG += sample.g;
                    blurB += sample.b;
                }
            }
            blurR /= 9.0;
            blurG /= 9.0;
            blurB /= 9.0;

            const auto& source = at(x, y);
            output.pixels[static_cast<std::size_t>(y * input.width + x)] = {
                toByte(source.r + (source.r - blurR) * amount),
                toByte(source.g + (source.g - blurG) * amount),
                toByte(source.b + (source.b - blurB) * amount),
            };
        }
    }
    return output;
}

Image readPpm(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Unable to open input image: " + path);
    }

    const std::string magic = readToken(input);
    if (magic != "P6") {
        throw std::runtime_error("Only binary PPM (P6) files are supported by the built-in reader");
    }

    const int width = std::stoi(readToken(input));
    const int height = std::stoi(readToken(input));
    const int maxValue = std::stoi(readToken(input));
    if (width <= 0 || height <= 0 || maxValue != 255) {
        throw std::runtime_error("Unsupported PPM dimensions or max value");
    }
    input.get();

    Image image{width, height, std::vector<RgbPixel>(static_cast<std::size_t>(width) * static_cast<std::size_t>(height))};
    input.read(reinterpret_cast<char*>(image.pixels.data()), static_cast<std::streamsize>(image.pixels.size() * sizeof(RgbPixel)));
    if (!input) {
        throw std::runtime_error("PPM pixel data is truncated: " + path);
    }
    return image;
}

void writePpm(const std::string& path, const Image& image) {
    validateImage(image);
    const auto parent = std::filesystem::path(path).parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("Unable to open output image: " + path);
    }
    output << "P6\n" << image.width << ' ' << image.height << "\n255\n";
    output.write(reinterpret_cast<const char*>(image.pixels.data()), static_cast<std::streamsize>(image.pixels.size() * sizeof(RgbPixel)));
}

} // namespace scuba
