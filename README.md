# Scuba Media Corrector

Scuba Media Corrector is an early Windows-focused C++ prototype for one-touch underwater color correction and grading. It targets scuba, snorkeling, marine research, and drone footage workflows where blue/green color casts, haze, and low contrast make media difficult to share without manual grading.

## Current prototype

The repository currently contains a portable C++20 processing core plus a command-line harness. The built-in reader/writer supports binary PPM (P6) images so the algorithm can be compiled and tested without requiring external codecs. The core API is intentionally codec-agnostic and ready to be connected to FFmpeg/OpenCV for JPG, PNG, RAW, and video frame ingestion.

## Implemented features

- One-touch automatic correction that estimates channel imbalance, boosts attenuated reds, reduces aqua dominance, and adds a warm underwater default grade.
- Intensity blending from 0% original to 100% corrected output.
- Manual grading controls for temperature, tint, highlights, shadows, whites, blacks, saturation, vibrance, aqua reduction, clarity, and sharpness.
- Batch folder processing through the CLI for supported image files.
- Split-screen before/after preview generation.

## Build and test

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## CLI usage

```bash
./build/scuba_corrector_cli --input samples/input.ppm --output corrected --split
./build/scuba_corrector_cli --input samples/batch --output corrected --intensity 0.8 --aqua-reduction 0.5 --vibrance 0.2
```

## Windows application roadmap

1. Add FFmpeg/OpenCV adapters for common still image formats and video frame streams.
2. Add DirectML/CUDA execution paths for depth-aware estimation, neural presets, and hardware encode/decode.
3. Build a WinUI 3 or Qt desktop shell with drag-and-drop import, preview scrubbing, sliders, presets, and export queues.
4. Add preset serialization so users can apply a custom grade to large batches from the same dive site or depth.
5. Package as a signed Windows installer with GPU capability detection and CPU fallback.
