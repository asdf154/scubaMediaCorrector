#include "scuba/ColorCorrection.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void printUsage() {
    std::cout << "Usage:\n"
              << "  scuba_corrector_cli --input <file-or-folder> --output <folder> [options]\n\n"
              << "Options:\n"
              << "  --intensity <0..1>       Automatic correction blend amount (default: 1.0)\n"
              << "  --temperature <-1..1>    Manual cool/warm shift\n"
              << "  --tint <-1..1>           Manual green/magenta shift\n"
              << "  --saturation <-1..1>     Saturation adjustment\n"
              << "  --vibrance <-1..1>       Vibrance adjustment\n"
              << "  --aqua-reduction <0..1>  Cyan/aqua reduction strength (default: 0.35)\n"
              << "  --clarity <0..1>         Local contrast strength\n"
              << "  --sharpness <0..1>       Sharpening strength\n"
              << "  --split                  Also writes before/after split preview files\n\n"
              << "Current built-in codec support is binary PPM (P6). The processing core is designed "
              << "to be wired to FFmpeg/OpenCV decoders for JPG, PNG, RAW, and video.\n";
}

std::string requireValue(int& index, int argc, char** argv) {
    if (index + 1 >= argc) {
        throw std::invalid_argument(std::string("Missing value for ") + argv[index]);
    }
    return argv[++index];
}

double parseDouble(int& index, int argc, char** argv) {
    return std::stod(requireValue(index, argc, argv));
}

bool isPpm(const std::filesystem::path& path) {
    return path.extension() == ".ppm" || path.extension() == ".PPM";
}

void processFile(const std::filesystem::path& inputPath,
                 const std::filesystem::path& outputFolder,
                 const scuba::CorrectionSettings& settings,
                 bool writeSplit) {
    scuba::ColorCorrectionEngine engine;
    const scuba::Image input = scuba::readPpm(inputPath.string());
    const scuba::Image corrected = engine.autoCorrect(input, settings);

    const std::filesystem::path outputPath = outputFolder / inputPath.filename();
    scuba::writePpm(outputPath.string(), corrected);

    if (writeSplit) {
        const auto preview = engine.splitComparison(input, corrected);
        const auto previewName = inputPath.stem().string() + "_split.ppm";
        scuba::writePpm((outputFolder / previewName).string(), preview);
    }
    std::cout << "Corrected " << inputPath << " -> " << outputPath << '\n';
}

} // namespace

int main(int argc, char** argv) {
    if (argc == 1) {
        printUsage();
        return 0;
    }

    try {
        std::filesystem::path inputPath;
        std::filesystem::path outputFolder;
        scuba::CorrectionSettings settings;
        bool writeSplit = false;

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--input") {
                inputPath = requireValue(i, argc, argv);
            } else if (arg == "--output") {
                outputFolder = requireValue(i, argc, argv);
            } else if (arg == "--intensity") {
                settings.intensity = parseDouble(i, argc, argv);
            } else if (arg == "--temperature") {
                settings.manual.temperature = parseDouble(i, argc, argv);
            } else if (arg == "--tint") {
                settings.manual.tint = parseDouble(i, argc, argv);
            } else if (arg == "--saturation") {
                settings.manual.saturation = parseDouble(i, argc, argv);
            } else if (arg == "--vibrance") {
                settings.manual.vibrance = parseDouble(i, argc, argv);
            } else if (arg == "--aqua-reduction") {
                settings.manual.aquaReduction = parseDouble(i, argc, argv);
            } else if (arg == "--clarity") {
                settings.manual.clarity = parseDouble(i, argc, argv);
            } else if (arg == "--sharpness") {
                settings.manual.sharpness = parseDouble(i, argc, argv);
            } else if (arg == "--split") {
                writeSplit = true;
            } else if (arg == "--help" || arg == "-h") {
                printUsage();
                return 0;
            } else {
                throw std::invalid_argument("Unknown argument: " + arg);
            }
        }

        if (inputPath.empty() || outputFolder.empty()) {
            throw std::invalid_argument("Both --input and --output are required");
        }

        if (std::filesystem::is_directory(inputPath)) {
            for (const auto& entry : std::filesystem::directory_iterator(inputPath)) {
                if (entry.is_regular_file() && isPpm(entry.path())) {
                    processFile(entry.path(), outputFolder, settings, writeSplit);
                }
            }
        } else {
            processFile(inputPath, outputFolder, settings, writeSplit);
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n\n";
        printUsage();
        return 1;
    }

    return 0;
}
