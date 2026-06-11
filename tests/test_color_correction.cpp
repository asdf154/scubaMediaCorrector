#include "scuba/ColorCorrection.h"

#include <cassert>
#include <filesystem>
#include <iostream>

namespace {

scuba::Image makeBlueCastImage() {
    return {
        2,
        2,
        {
            {30, 105, 190},
            {35, 112, 198},
            {20, 95, 170},
            {40, 120, 210},
        },
    };
}

void testAutoCorrectionReducesBlueCast() {
    const scuba::Image image = makeBlueCastImage();
    scuba::ColorCorrectionEngine engine;
    scuba::CorrectionSettings settings;
    const scuba::Image corrected = engine.autoCorrect(image, settings);

    assert(corrected.width == image.width);
    assert(corrected.height == image.height);
    assert(corrected.pixels.size() == image.pixels.size());
    assert(corrected.pixels.front().r > image.pixels.front().r);
    assert(corrected.pixels.front().b < image.pixels.front().b);
}

void testIntensityZeroKeepsOriginalWithoutSharpening() {
    const scuba::Image image = makeBlueCastImage();
    scuba::ColorCorrectionEngine engine;
    scuba::CorrectionSettings settings;
    settings.intensity = 0.0;
    const scuba::Image corrected = engine.autoCorrect(image, settings);
    assert(corrected.pixels.front().r == image.pixels.front().r);
    assert(corrected.pixels.front().g == image.pixels.front().g);
    assert(corrected.pixels.front().b == image.pixels.front().b);
}

void testSplitComparisonUsesAfterOnRightSide() {
    const scuba::Image before = makeBlueCastImage();
    scuba::Image after = before;
    after.pixels[1] = {255, 0, 0};
    after.pixels[3] = {255, 0, 0};

    scuba::ColorCorrectionEngine engine;
    const scuba::Image split = engine.splitComparison(before, after, 0.5);
    assert(split.pixels[0].b == before.pixels[0].b);
    assert(split.pixels[1].r == 255);
    assert(split.pixels[3].r == 255);
}

void testPpmRoundTrip() {
    const scuba::Image image = makeBlueCastImage();
    const std::filesystem::path folder = std::filesystem::temp_directory_path() / "scuba_corrector_tests";
    const std::filesystem::path file = folder / "sample.ppm";
    scuba::writePpm(file.string(), image);
    const scuba::Image read = scuba::readPpm(file.string());
    assert(read.width == image.width);
    assert(read.height == image.height);
    assert(read.pixels[2].g == image.pixels[2].g);
}

} // namespace

int main() {
    testAutoCorrectionReducesBlueCast();
    testIntensityZeroKeepsOriginalWithoutSharpening();
    testSplitComparisonUsesAfterOnRightSide();
    testPpmRoundTrip();
    std::cout << "All color correction tests passed\n";
    return 0;
}
