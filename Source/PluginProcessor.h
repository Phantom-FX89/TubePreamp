#pragma once
#include <JuceHeader.h>

// ============================================================================
//  TubePreampProcessor
//
//  This class IS the plugin. The DAW (host) creates one instance of it and
//  calls processBlock() for every chunk of audio — around 86 times per second
//  at 44.1kHz with a 512-sample buffer.
//
//  Audio signal flow inside processBlock():
//
//    DAW Input Audio
//        │
//        ▼
//    ┌─────────────────────────────────┐
//    │  STEP 1: MIC LEVEL GAIN         │  Linear multiply (converted from dB)
//    └──────────────┬──────────────────┘
//                   │
//                   ▼
//    ┌─────────────────────────────────┐
//    │  STEP 2: TUBE SATURATION        │  tanh() waveshaping (drive > 0)
//    │          DRIVE                  │  or subtle even-harmonic warmth (drive < 0)
//    └──────────────┬──────────────────┘
//                   │
//                   ▼
//    ┌─────────────────────────────────┐
//    │  STEP 3: TONE EQ                │  3 biquad IIR filters in series
//    │  (SSL 4K / Neve 1073 /          │  Low shelf → Peak → High shelf
//    │   Focusrite ISA, or bypass)     │
//    └──────────────┬──────────────────┘
//                   │
//                   ▼
//             DAW Output Audio
//
// ============================================================================

class TubePreampProcessor  :  public juce::AudioProcessor
{
public:
    // ── Constructor / Destructor ──────────────────────────────────────────
    TubePreampProcessor();
    ~TubePreampProcessor() override;

    // ── Core JUCE overrides ───────────────────────────────────────────────

    // prepareToPlay() is called once when the DAW starts up the plugin, or
    // any time settings change (sample rate, buffer size). Use it to:
    //   • allocate buffers
    //   • calculate filter coefficients  ← we do this
    //   • set up parameter smoothing     ← we do this
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;

    // releaseResources() is called when the DAW unloads the plugin.
    void releaseResources() override;

    // processBlock() is THE HOT PATH. Called ~86x per second.
    // Rules: NO memory allocation, NO locks, NO system calls.
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // ── GUI ───────────────────────────────────────────────────────────────
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                    { return true; }

    // ── Plugin identity ───────────────────────────────────────────────────
    const juce::String getName() const override        { return "Tube Preamp"; }
    bool  acceptsMidi()  const override                { return false; }
    bool  producesMidi() const override                { return false; }
    bool  isMidiEffect() const override                { return false; }
    double getTailLengthSeconds() const override       { return 0.0; }

    // ── Presets (kept minimal for now) ────────────────────────────────────
    int   getNumPrograms()    override                 { return 1; }
    int   getCurrentProgram() override                 { return 0; }
    void  setCurrentProgram (int)  override            {}
    const juce::String getProgramName (int) override   { return {}; }
    void  changeProgramName (int, const juce::String&) override {}

    // ── Session state (save/load with the DAW project) ────────────────────
    void getStateInformation (juce::MemoryBlock& destData)  override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ── Public parameter tree ─────────────────────────────────────────────
    // Both the Processor and the Editor access parameters through this object.
    // "APVTS" = AudioProcessorValueTreeState — JUCE's standard parameter container.
    // It manages thread-safe access, undo/redo, and session serialization.
    juce::AudioProcessorValueTreeState apvts;

private:
    // ── Parameter layout factory ──────────────────────────────────────────
    // Called once in the constructor to define all our parameters.
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    // ── Filter storage ────────────────────────────────────────────────────
    //
    // We have 3 tone modes × 3 EQ bands × 2 channels = 18 biquad filters.
    //
    //   toneFilters[mode][band][channel]
    //     mode:    0=SSL 4K,  1=Neve 1073,  2=Focusrite ISA
    //     band:    0=LowShelf, 1=Peak,       2=HighShelf
    //     channel: 0=Left,    1=Right
    //
    // A "biquad" filter is a second-order IIR filter — the workhorse of
    // digital audio EQ. It uses 5 coefficients (b0,b1,b2,a1,a2) and
    // maintains 2 state variables (memory of past samples).
    //
    using IIRFilter = juce::dsp::IIR::Filter<float>;
    IIRFilter toneFilters[3][3][2];

    // Helper to compute and assign EQ coefficients for all 3 tone modes.
    // Called once in prepareToPlay() and any time sample rate changes.
    void updateToneFilterCoefficients (double sampleRate);

    // ── Parameter smoothing ───────────────────────────────────────────────
    //
    // WHY DO WE SMOOTH?
    //   When a knob changes value, the parameter jumps from old to new.
    //   Applying that jump directly to the audio causes "zipper noise" —
    //   an audible click or stepping artifact.
    //
    //   SmoothedValue fixes this: instead of jumping, it glides linearly
    //   from the old value to the new value over ~20 milliseconds.
    //   At 44.1kHz with a 512-sample buffer, 20ms = ~882 samples of glide.
    //
    juce::SmoothedValue<float> smoothedGainLinear;  // Mic gain (linear, not dB)
    juce::SmoothedValue<float> smoothedDrive;        // Drive (-1.0 to +1.0)

    // ── Core tube saturation DSP ──────────────────────────────────────────
    //
    // processTubeSaturation() is called per-sample inside processBlock().
    // It applies the nonlinear waveshaping that creates tube harmonic character.
    //
    // "inline" hints to the compiler to embed this code directly at the call
    // site rather than making a function call — important for performance
    // since we call this millions of times per second.
    //
    // "noexcept" promises this function never throws exceptions — the compiler
    // can optimize more aggressively knowing that.
    //
    inline float processTubeSaturation (float input, float drive) noexcept;

    // Tracks the current sample rate so filters can be rebuilt if it changes.
    double currentSampleRate = 44100.0;

    // JUCE macro: adds a leak detector and disables accidental copying of this class.
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TubePreampProcessor)
};
