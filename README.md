# Tube Preamp Plugin

A tube preamp emulation plugin for macOS, targeting AU (Logic Pro) and VST3 (Ableton, Reaper, etc.).

---

## What It Does

This plugin emulates the sonic character of a tube microphone preamplifier, with three
switchable console EQ curves based on the SSL 4000, Neve 1073, and Focusrite ISA preamps.

**Signal flow:**

```
Input → Mic Level Gain → Tube Saturation (Drive) → Tone EQ → Output
```

---

## Controls

### DRIVE (bipolar knob — center = clean)

| Position | Sound | What's happening in the code |
|----------|-------|------------------------------|
| Left (−1.0) | Subtle warmth | Asymmetric x² waveshaping → trace 2nd harmonic |
| Center (0.0) | Clean, transparent | Signal passes through unchanged |
| Right (+1.0) | Heavy tube push | `tanh(k·x)/tanh(k)` — soft-clips peaks, generates even harmonics |

**Double-click to reset to center (clean).**

The positive side uses the hyperbolic tangent function `tanh()`, which is mathematically
ideal for tube saturation modeling: it's linear at low levels and soft-clips smoothly at
high levels, generating the even harmonics (2nd, 4th) that give tubes their warmth.

### MIC LEVEL (gain knob)

Range: −20 dB to +60 dB. Unity (0 dB) means no level change.
- Use negative values to attenuate hot line-level signals
- Use high positive values to boost quiet microphone signals

**Double-click to reset to 0 dB (unity gain).**

### Console Character Buttons

Three mutually exclusive tone-shaping modes. **Click the active button again to turn it off.**

| Button | Frequency Signature | Character |
|--------|---------------------|-----------|
| **SSL 4K** | +1.5 dB @80Hz, +2.0 dB @3.5kHz, +1.5 dB @12kHz | Punchy, forward, modern |
| **NEVE 1073** | +3.0 dB @60Hz, +2.5 dB @3.2kHz, +1.0 dB @10kHz | Warm, rich, vintage |
| **FOCUSRITE** | +1.5 dB @80Hz, +1.5 dB @5kHz, +2.5 dB @16kHz | Clean, extended air |

Each mode uses three biquad IIR filters in series (low shelf → peak → high shelf),
designed using the classic RBJ (Robert Bristow-Johnson) cookbook formulas.

---

## Prerequisites

| Tool | Version | Install |
|------|---------|---------|
| macOS | 11.0+ (Big Sur) | — |
| Xcode | 14.0+ | Mac App Store |
| CMake | 3.22+ | `brew install cmake` |
| JUCE | 7.0+ | Git submodule (see below) |

---

## First-Time Setup

```bash
# 1. Navigate to the TubePreamp folder
cd /path/to/TubePreamp

# 2. Initialize git (if this isn't already a git repo)
git init

# 3. Add JUCE as a git submodule
git submodule add https://github.com/juce-framework/JUCE.git JUCE
git submodule update --init --recursive

# 4. Make the install script executable
chmod +x install.sh
```

**Before building**, open `CMakeLists.txt` and change these four lines to your own values:

```cmake
COMPANY_NAME                "YourStudio"           # ← Your name or company
PLUGIN_MANUFACTURER_CODE    "Ystd"                 # ← Unique 4-char code (YOU pick this)
PLUGIN_CODE                 "Tpre"                 # ← Unique 4-char code (YOU pick this)
BUNDLE_ID                   "com.yourstudio.tubepreamp"  # ← Your reverse domain
```

> **Why unique codes?** If two plugins share the same PLUGIN_CODE, the DAW may load
> the wrong one. Choose codes that are very unlikely to conflict.

---

## Build & Install

```bash
./install.sh
```

That's it. The script will:
1. ✅ Validate your environment (Xcode, CMake, JUCE)
2. 🔧 Configure the CMake/Xcode build system
3. ⚙️  Build the Universal Binary (arm64 + x86_64)
4. 📦 Install AU to `~/Library/Audio/Plug-Ins/Components/`
5. 📦 Install VST3 to `~/Library/Audio/Plug-Ins/VST3/`
6. 🔄 Refresh the audio component registry

**For a clean rebuild** (if something seems wrong):
```bash
./install.sh --clean
```

---

## Project Structure

```
TubePreamp/
├── CMakeLists.txt              ← Build configuration
├── install.sh                  ← Build & deploy script
├── README.md                   ← This file
├── JUCE/                       ← JUCE framework (git submodule)
└── Source/
    ├── PluginProcessor.h       ← DSP class declaration
    ├── PluginProcessor.cpp     ← All audio processing (saturation, EQ, gain)
    ├── PluginEditor.h          ← GUI class declaration
    └── PluginEditor.cpp        ← GUI drawing and control logic
```

---

## DSP Architecture Reference

### How the Tube Saturation Works

The drive algorithm uses `tanh()` (hyperbolic tangent), the mathematically
ideal soft-clipper for tube modeling:

```
y = tanh(k · x) / tanh(k)

where:
  x = input sample (−1.0 to +1.0)
  k = drive amount (scales from 1.0 to 12.0 as drive goes 0 → 1)
  tanh(k) = normalization factor (keeps output within ±1.0)
```

At low levels, `tanh(x) ≈ x` (linear — no distortion).
At high levels, `tanh(x) → ±1` (soft peak compression).
The bend in the curve introduces harmonic content. Because `tanh` is
an odd function with a smooth shape, it generates *even harmonics* when
combined with the slight asymmetry of real tube circuits — these are musically
consonant (octaves, fifths) and sound "warm."

### How the Biquad EQ Filters Work

Each tone mode uses three second-order IIR (biquad) filters in series.
A biquad uses only 5 multiply-add operations per sample — very efficient.

The filter equation:
```
y[n] = b0·x[n] + b1·x[n−1] + b2·x[n−2]
                − a1·y[n−1] − a2·y[n−2]
```
Where `b0,b1,b2,a1,a2` are the coefficients (computed from frequency, gain, Q).
`x[n−1], x[n−2]` are the previous two input samples.
`y[n−1], y[n−2]` are the previous two output samples.
This feedback ("IIR") creates the resonant shelf/peak shapes.

### Parameter Smoothing (No Zipper Noise)

When a knob changes value, applying the new value instantly to every sample
causes a "zipper noise" — an audible click or stepping artifact. We prevent
this using `juce::SmoothedValue`, which glides from the old value to the new
value over 20 milliseconds (~882 samples at 44.1kHz).

---

## Next Development Steps

When you're ready to expand the plugin, here are good next moves:

1. **Oversampling** — Add 2x or 4x oversampling to the saturation stage to reduce
   aliasing artifacts at high drive settings. JUCE's `juce::dsp::Oversampling` class
   handles this with just a few lines of code.

2. **Custom knob graphics** — Override `drawRotarySlider()` in a custom LookAndFeel
   to draw skeuomorphic knobs that look like real hardware.

3. **Output gain** — Add a makeup gain knob after saturation (compensates for the
   loudness increase that saturation causes at positive drive values).

4. **Harmonic analyzer** — Add a spectrum display to show the harmonic content the
   saturation is generating. Great for understanding the DSP and great marketing.

5. **AAX format** — Add AAX support for Pro Tools. Requires the Avid AAX SDK and
   PACE anti-piracy signing. See the companion analysis document for details.
