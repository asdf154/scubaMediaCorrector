# Product Requirements Document: Scuba Media Corrector

## 1. Executive summary

Scuba Media Corrector is a Windows desktop application for automatic color correction and grading of underwater and low-light photos/videos. The app provides a one-touch workflow similar in spirit to Dive+, AquaColorFix, and PixlFix while prioritizing desktop-grade batch processing, original-resolution export, and manual override controls.

## 2. Target audience and problem statement

### Target users

- Scuba divers and snorkelers who want quick, natural-looking shareable media.
- Marine biologists and researchers who need repeatable correction on large dive batches.
- Drone and action-camera operators working with hazy, low-light, or blue/green-dominant footage.

### Core problem

Underwater media loses red and warm wavelengths quickly, leaving photos and footage with heavy cyan/blue/green casts, low contrast, and haze. Mobile apps can help, but desktop users need a fast Windows workflow that preserves resolution, supports batch jobs, and avoids the complexity of professional grading suites.

## 3. Core user flows

### 3.1 One-touch auto correction

1. User drags files or a folder into the app.
2. The processing engine estimates the dominant underwater cast and balances RGB channels.
3. The app reduces aqua/cyan dominance, recovers warm tones, improves shadows, and applies a natural saturation/vibrance lift.
4. User adjusts an intensity slider from 0% to 100% to blend between original and corrected media.

### 3.2 Manual fine tuning

After auto correction, users can refine the output with these controls:

- Temperature and tint.
- Highlights, shadows, blacks, and whites.
- Saturation and vibrance.
- Aqua/cyan reduction.
- Sharpness and clarity for haze reduction.

### 3.3 Batch processing

Users can import an entire folder from a dive, apply automatic correction or a saved preset, preview a representative item, and export the full batch to a selected destination.

### 3.4 Split-screen comparison

Users can validate the correction with before/after side-by-side views or a swipe-style divider before exporting.

## 4. Technical requirements

### 4.1 Desktop platform

- Windows 10 and Windows 11.
- C++20 processing core.
- FFmpeg/OpenCV integration for production image/video decode, processing, and encode.
- DirectML or CUDA acceleration for future depth estimation, neural color models, and hardware-assisted 4K video export.

### 4.2 Prototype scope in this repository

- Portable C++ correction engine with no mandatory third-party runtime dependencies.
- CLI harness for batch execution against binary PPM test images.
- Unit tests covering automatic correction, intensity blending, split preview generation, and PPM I/O.

## 5. Success metrics

- Correct a representative underwater image in one action with visibly reduced blue/green cast.
- Process a folder with consistent parameters without per-file manual work.
- Preserve source dimensions in corrected output.
- Provide manual controls that map directly to common grading concepts.

## 6. Future milestones

1. Integrate FFmpeg/OpenCV codecs for JPG, PNG, HEIC, RAW proxies, MP4, MOV, and common action-camera formats.
2. Add GPU-accelerated color transforms and hardware video encoding.
3. Implement a Windows UI with drag-and-drop, preview playback, sliders, presets, and export queue management.
4. Add optional AI/depth-aware models to adapt correction strength by estimated water depth and scene content.
