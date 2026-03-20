# ARCHITECTURE

## Overview
TubePreamp is an audio plugin (AU/VST3) built using the JUCE framework in C++. 

## Directory Structure
- `Source/PluginProcessor.cpp / .h`: DSP backend. Handles all logic for the audio chain.
- `Source/PluginEditor.cpp / .h`: GUI frontend. Logic for the knobs and buttons.

## DSP Architecture Flow
`Input → Mic Level Gain → Tube Saturation (Drive) → Tone EQ → Output`

### Core Components
1. **Tube Saturation (Drive):**
   - Utilizes `tanh(k * x) / tanh(k)` as a soft-clipper to model tube distortion (generating even harmonics).
   - Clean at `k=0`, adds warmth down to `-1.0`, heavy tube push up to `+1.0`.

2. **Console Character EQ (Biquad Filters):**
   - Employs Robert Bristow-Johnson (RBJ) cookbook filters.
   - Three biquads in series: Low Shelf → Peak → High Shelf.
   - Distinct curves for SSL 4K, NEVE 1073, and FOCUSRITE.

3. **Parameter Smoothing:**
   - `juce::SmoothedValue` prevents zipper noise during knob adjustment by smoothing transitions over 20ms.
