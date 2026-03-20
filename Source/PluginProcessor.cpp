#include "PluginProcessor.h"
#include "PluginEditor.h"

// ============================================================================
//  Parameter IDs
//
//  We define these as string constants rather than typing the strings everywhere.
//  If you make a typo in a string, the compiler won't catch it — but using a
//  constant, the compiler WILL catch a typo. This is a common C++ practice.
// ============================================================================
namespace ParamID
{
    static const juce::String micGain  = "micGain";
    static const juce::String drive    = "drive";
    static const juce::String toneMode = "toneMode";
}

// ============================================================================
//  createParameters()
//
//  Defines all three plugin parameters and returns them as a ParameterLayout.
//  This is called once from the constructor.
//
//  The APVTS (AudioProcessorValueTreeState) uses this layout to create and
//  manage the parameters. It handles:
//    • Thread-safe access (audio thread reads, UI thread writes)
//    • Host automation (DAW can record/playback knob movements)
//    • Session persistence (values saved with the DAW project)
// ============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout TubePreampProcessor::createParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout params;

    // ── Mic Level Gain: -20 dB to +60 dB ─────────────────────────────────
    // This is a wide range typical of real microphone preamps:
    //   -20 dB = quieting a signal (for line-level inputs)
    //    0 dB  = unity gain (no change)
    //   +60 dB = maximum boost (for very quiet microphones)
    //
    // NormalisableRange sets the step size to 0.1 dB increments.
    params.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamID::micGain, 1 },
        "Mic Gain",
        juce::NormalisableRange<float>(-20.0f, 60.0f, 0.1f),
        0.0f,                                     // default: 0 dB (no change)
        juce::AudioParameterFloatAttributes().withLabel("dB")
    ));

    // ── Drive: -1.0 to +1.0 (bipolar, center = clean) ────────────────────
    // This controls the tube saturation character:
    //   -1.0 = subtle even-harmonic "Class A warmth" (very gentle)
    //    0.0 = clean, no saturation (center position on the knob)
    //   +1.0 = heavy tube saturation (pushed hard into the transfer function)
    params.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamID::drive, 1 },
        "Drive",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f),
        0.0f                                      // default: center (clean)
    ));

    // ── Tone Mode: 0=Off, 1=SSL 4K, 2=Neve 1073, 3=Focusrite ────────────
    // Integer parameter for the three console character EQ modes.
    // 0 = bypass (no EQ), 1-3 = one of the three tone modes.
    params.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{ ParamID::toneMode, 1 },
        "Tone Mode",
        0,   // min (Off)
        3,   // max (Focusrite)
        0    // default (Off)
    ));

    return params;
}

// ============================================================================
//  Constructor
//
//  The BusesProperties() call tells the DAW what audio I/O this plugin uses:
//    withInput  ("Input",  stereo) = 2-channel audio input
//    withOutput ("Output", stereo) = 2-channel audio output
//
//  The APVTS is initialized here using our parameter layout factory.
// ============================================================================
TubePreampProcessor::TubePreampProcessor()
     : AudioProcessor (BusesProperties()
                         .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                         .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       apvts (*this, nullptr, "TubePreampState", createParameters())
{
    // Nothing extra — filter setup happens in prepareToPlay()
}

TubePreampProcessor::~TubePreampProcessor() {}

// ============================================================================
//  prepareToPlay()
//
//  Called once when the DAW loads the plugin, and again if the user changes
//  the sample rate or buffer size in their DAW settings.
//
//  This is where we:
//    1. Set up SmoothedValue ramping
//    2. Reset filter state to zero
//    3. Calculate all EQ filter coefficients
// ============================================================================
void TubePreampProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // ── Set up parameter smoothing ────────────────────────────────────────
    // "0.020 seconds" = 20 milliseconds of ramp time.
    // If the user moves a knob from 0.0 to 1.0, the gain value will glide
    // smoothly over 20ms instead of jumping. This eliminates zipper noise.
    const double smoothingTimeSec = 0.020;
    smoothedGainLinear.reset (sampleRate, smoothingTimeSec);
    smoothedDrive.reset      (sampleRate, smoothingTimeSec);

    // Set initial values from the current APVTS parameter values.
    // ".load()" does an atomic (thread-safe) read of the parameter's current value.
    float gainDB = apvts.getRawParameterValue (ParamID::micGain)->load();
    smoothedGainLinear.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (gainDB));
    // juce::Decibels::decibelsToGain(x) converts dB to linear: 10^(x/20)
    //   0 dB → 1.0 (unity gain)
    //  20 dB → 10.0 (10x amplitude)
    // -20 dB → 0.1 (1/10 amplitude)

    float drive = apvts.getRawParameterValue (ParamID::drive)->load();
    smoothedDrive.setCurrentAndTargetValue (drive);

    // ── Reset all filter state ────────────────────────────────────────────
    // Each biquad filter has internal memory of past samples (its "state").
    // We zero out that memory so there's no audio leftover from a previous session.
    for (int mode = 0; mode < 3; ++mode)
        for (int band = 0; band < 3; ++band)
            for (int ch = 0; ch < 2; ++ch)
                toneFilters[mode][band][ch].reset();

    // ── Compute EQ coefficients for all tone modes ────────────────────────
    // Coefficients depend on the sample rate, so we must (re)compute them here.
    updateToneFilterCoefficients (sampleRate);
}

void TubePreampProcessor::releaseResources()
{
    // Filters clean themselves up — nothing to do here.
}

// ============================================================================
//  updateToneFilterCoefficients()
//
//  This function sets up the EQ curves for each of the three tone modes.
//  Each mode has three biquad filters in series:
//
//    Band 0: Low shelf  — boosts or shapes the bass/low-mid region
//    Band 1: Peak EQ    — adds a presence bump in the upper midrange
//    Band 2: High shelf — adds "air" or top-end extension
//
//  WHY BIQUAD FILTERS?
//  A biquad is a second-order IIR filter. "IIR" = Infinite Impulse Response,
//  meaning its output feeds back into its own input. This creates resonance
//  and frequency shaping with only 5 multiplication operations per sample —
//  incredibly efficient.
//
//  The juce::dsp::IIR::Coefficients factory methods use the classic "RBJ cookbook"
//  formulas (by Robert Bristow-Johnson), the industry standard for EQ design.
//
//  ABOUT THE TONE CHARACTERS:
//  These curves are based on published analysis of the original hardware:
//
//  SSL 4000 G/E:    Punchy, modern. Tight low-end weight, forward presence
//                   at 3.5kHz, extended airy top. Classic pop/rock character.
//
//  Neve 1073:       Warm, rich, vintage. Fuller bottom at 60Hz (transformer
//                   coupling), smooth presence at 3.2kHz, gentle top.
//                   "Musical" in a way that just makes everything sound better.
//
//  Focusrite ISA:   Clean, transparent, extended. Tight low shelf, precise
//                   presence at 5kHz, very airy top at 16kHz. More accurate
//                   and less colored than the Neve — different tool, same class.
// ============================================================================
void TubePreampProcessor::updateToneFilterCoefficients (double sampleRate)
{
    // ── Helper lambdas ────────────────────────────────────────────────────
    // These wrap the JUCE coefficient factories to keep the code below readable.
    //
    // Parameters:
    //   freq   = corner/center frequency in Hz
    //   gainDB = boost amount in dB (positive = boost, negative = cut)
    //   Q      = resonance / selectivity
    //               0.5  = very broad shelf
    //               0.71 = Butterworth (maximally flat, no resonance peak)
    //               1.0  = moderate resonance
    //               2.0+ = narrow/peaky

    auto makeLowShelf = [&] (double freq, float gainDB, float Q)
    {
        return juce::dsp::IIR::Coefficients<float>::makeLowShelf (
            sampleRate, freq, Q,
            juce::Decibels::decibelsToGain (gainDB)  // JUCE wants LINEAR gain, not dB
        );
    };

    auto makeHighShelf = [&] (double freq, float gainDB, float Q)
    {
        return juce::dsp::IIR::Coefficients<float>::makeHighShelf (
            sampleRate, freq, Q,
            juce::Decibels::decibelsToGain (gainDB)
        );
    };

    auto makePeak = [&] (double freq, float gainDB, float Q)
    {
        return juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            sampleRate, freq, Q,
            juce::Decibels::decibelsToGain (gainDB)
        );
    };

    // ── MODE 0: SSL 4000 G/E Console Character ────────────────────────────
    // Reference: SSL G+ and E-series console preamp measurements.
    // Character: Punchy, forward, defined. The "modern rock" console.
    {
        const int m = 0;

        //                           freq       gain    Q
        auto lowShelf  = makeLowShelf  (80.0,  +1.5f,  0.71f);  // +1.5 dB @ 80 Hz — weight
        auto peak      = makePeak      (3500.0, +2.0f,  0.80f);  // +2.0 dB @ 3.5kHz — presence
        auto highShelf = makeHighShelf (12000.0,+1.5f,  0.71f);  // +1.5 dB @ 12kHz  — air

        for (int ch = 0; ch < 2; ++ch)
        {
            // The dereference syntax:
            //   toneFilters[m][band][ch].coefficients  is a smart pointer (juce::ReferenceCountedObjectPtr)
            //   *toneFilters[...].coefficients          dereferences it to get the Coefficients object
            //   = *lowShelf                             copies the new coefficient values into it
            *toneFilters[m][0][ch].coefficients = *lowShelf;
            *toneFilters[m][1][ch].coefficients = *peak;
            *toneFilters[m][2][ch].coefficients = *highShelf;
        }
    }

    // ── MODE 1: Neve 1073 Character ───────────────────────────────────────
    // Reference: Neve 1073 module preamp + EQ stage measurements.
    // Character: Warm, full, vintage. The "classic rock/soul" sound.
    // The big bottom end comes from transformer saturation at ~60Hz.
    {
        const int m = 1;

        //                           freq       gain    Q
        auto lowShelf  = makeLowShelf  (60.0,  +3.0f,  0.71f);  // +3.0 dB @ 60 Hz  — richness
        auto peak      = makePeak      (3200.0, +2.5f,  0.70f);  // +2.5 dB @ 3.2kHz — smooth presence
        auto highShelf = makeHighShelf (10000.0,+1.0f,  0.71f);  // +1.0 dB @ 10kHz  — gentle top

        for (int ch = 0; ch < 2; ++ch)
        {
            *toneFilters[m][0][ch].coefficients = *lowShelf;
            *toneFilters[m][1][ch].coefficients = *peak;
            *toneFilters[m][2][ch].coefficients = *highShelf;
        }
    }

    // ── MODE 2: Focusrite ISA Character ───────────────────────────────────
    // Reference: Focusrite ISA One/ISA 828 transformer preamp measurements.
    // Character: Clean, transparent, extended top. "Studio accuracy" with musicality.
    {
        const int m = 2;

        //                           freq       gain    Q
        auto lowShelf  = makeLowShelf  (80.0,  +1.5f,  0.71f);  // +1.5 dB @ 80 Hz  — tight weight
        auto peak      = makePeak      (5000.0, +1.5f,  1.20f);  // +1.5 dB @ 5kHz   — upper clarity
        auto highShelf = makeHighShelf (16000.0,+2.5f,  0.71f);  // +2.5 dB @ 16kHz  — extended air

        for (int ch = 0; ch < 2; ++ch)
        {
            *toneFilters[m][0][ch].coefficients = *lowShelf;
            *toneFilters[m][1][ch].coefficients = *peak;
            *toneFilters[m][2][ch].coefficients = *highShelf;
        }
    }
}

// ============================================================================
//  processTubeSaturation()
//
//  This is called once per sample inside processBlock(). It applies the
//  nonlinear transfer function that gives tubes their characteristic warmth.
//
//  HOW TUBE SATURATION WORKS IN THE REAL WORLD:
//    A tube (vacuum tube/valve) amplifies audio linearly at low levels.
//    But as the signal gets louder, the tube starts to run out of headroom
//    and begins to clip — but not harshly like a transistor. Instead, it
//    "bends" the peaks of the waveform smoothly.
//
//    This smooth bending introduces harmonics (new frequency components)
//    that weren't in the original signal. Tubes tend to produce EVEN harmonics
//    (2nd, 4th, 6th...) — these are musically consonant intervals (octaves,
//    etc.) and sound "warm" to our ears.
//
//  HOW WE MODEL IT IN CODE (Drive > 0):
//    We use the hyperbolic tangent function: tanh(x)
//    This is mathematically perfect for soft-clipping because:
//      • At low levels (x near 0): tanh(x) ≈ x   →  linear (no distortion)
//      • At high levels (x large): tanh(x) → ±1   →  soft peak compression
//      • It's smooth and continuous — no harsh corners
//
//    We scale the input by a drive factor k before passing to tanh(),
//    then divide by tanh(k) to normalize output level:
//
//      y = tanh(k * x) / tanh(k)
//
//    At k=1: very subtle (almost clean)
//    At k=6: moderate tube push
//    At k=12: pushed hard, clear saturation on peaks
//
//  HOW WE MODEL IT (Drive < 0):
//    Negative drive gives a subtle asymmetric second-harmonic character —
//    the "Class A whisper" where the circuit is just barely doing something.
//    We use: y = x + amount * x² * sign(x)
//    The x² term is always positive, so adding it to positive half-cycles
//    and subtracting from negative half-cycles creates asymmetry →
//    which produces 2nd harmonic content (the most musical harmonic).
// ============================================================================
inline float TubePreampProcessor::processTubeSaturation (float input, float drive) noexcept
{
    // If drive is essentially zero (within floating-point rounding), pass through.
    if (std::abs (drive) < 0.001f)
        return input;

    if (drive > 0.0f)
    {
        // ── Positive drive: tube saturation via tanh ───────────────────────
        //
        // Map drive (0.0 → 1.0) to k (1.0 → 12.0):
        //   At drive=0.01 → k=1.11  = barely a touch
        //   At drive=0.5  → k=6.5   = moderate push
        //   At drive=1.0  → k=12.0  = heavy saturation
        //
        const float k = 1.0f + drive * 11.0f;

        // Apply tanh saturation:
        //   tanh(k * x)  = the saturated signal
        //   tanh(k)      = the max value tanh(k*x) can reach (when x=1.0)
        //   dividing normalizes so output peaks stay at ±1.0
        //   (prevents loudness increase as drive goes up)
        //
        const float saturated  = std::tanh (k * input);
        const float normalizer = 1.0f / std::tanh (k);

        return saturated * normalizer;
    }
    else
    {
        // ── Negative drive: second-harmonic "Class A warmth" ──────────────
        //
        // Very subtle. Even at drive=-1.0, the effect is just 15% distortion.
        // This creates a whisper of even harmonics — you often "feel" this
        // more than hear it clearly.
        //
        // Formula: y = x + amount * x² * sign(x)
        //   sign(x) = +1 for positive samples, -1 for negative samples
        //   This creates asymmetry → even harmonics
        //
        const float amount = (-drive) * 0.15f;  // scale: max 0.15 at drive=-1.0
        const float sign   = (input > 0.0f) ? 1.0f : -1.0f;

        return input + amount * input * input * sign;
    }
}

// ============================================================================
//  processBlock()
//
//  THE HOT PATH — called by the DAW approximately 86 times per second
//  (at 44.1kHz, 512-sample buffer). Every sample of audio flows through here.
//
//  buffer  = the audio data. Contains [numChannels][numSamples] floats.
//            Audio values range from -1.0 to +1.0 (normalized).
//  midiMessages = ignored (we're not a MIDI effect)
//
//  REAL-TIME SAFETY RULES (break these = audio glitches):
//    ✅ OK:  read/write the buffer, call math functions, read atomics
//    ❌ NO:  malloc/new/delete, std::vector::push_back, mutex locks,
//            file I/O, printf, sleep, any system call with unknown timing
// ============================================================================
void TubePreampProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& /*midiMessages*/)
{
    // ScopedNoDenormals: sets CPU hardware flags to flush "denormal" floats
    // to zero. Denormals are tiny floating-point values (< ~1e-38) that cost
    // 150-200x more CPU cycles than normal floats. They appear in IIR filter
    // feedback loops during silence. This one line prevents CPU spikes.
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    // ── Read current parameter targets ─────────────────────────────────────
    // .load() is an atomic read — safe to call from the audio thread.
    // Note: we only read toneMode once per block (it doesn't need smoothing —
    // changing tone mode mid-block would create a click anyway).
    const float targetGainDB = apvts.getRawParameterValue (ParamID::micGain)->load();
    const float targetDrive  = apvts.getRawParameterValue (ParamID::drive)->load();
    const int   toneMode     = (int)apvts.getRawParameterValue (ParamID::toneMode)->load();

    // Tell the SmoothedValues what values to ramp toward.
    // They'll glide there over the next ~20ms of samples.
    smoothedGainLinear.setTargetValue (juce::Decibels::decibelsToGain (targetGainDB));
    smoothedDrive.setTargetValue (targetDrive);

    // Pre-cache channel pointers to avoid a virtual function call per sample.
    // jmin(numChannels, 2) = process at most 2 channels (stereo).
    const int channelsToProcess = juce::jmin (numChannels, 2);
    float* channels[2] = { nullptr, nullptr };
    for (int ch = 0; ch < channelsToProcess; ++ch)
        channels[ch] = buffer.getWritePointer (ch);

    // ── Sample-by-sample processing ────────────────────────────────────────
    //
    // WHY outer loop = samples, inner loop = channels?
    //   SmoothedValue::getNextValue() advances the ramp by ONE step each call.
    //   We want BOTH channels to use the SAME gain and drive value at each
    //   sample moment. If we put channels in the outer loop, each channel
    //   would consume different smoothed values — that would create a stereo
    //   imbalance artifact while a knob is being moved.
    //
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Advance smoothing by one step and get current interpolated values.
        const float gain  = smoothedGainLinear.getNextValue();
        const float drive = smoothedDrive.getNextValue();

        for (int ch = 0; ch < channelsToProcess; ++ch)
        {
            // ── STEP 1: Apply Mic Level gain ────────────────────────────
            float x = channels[ch][sample] * gain;

            // ── STEP 2: Apply tube saturation ───────────────────────────
            x = processTubeSaturation (x, drive);

            // ── STEP 3: Apply tone EQ (if enabled) ─────────────────────
            // toneMode 0 = Off (bypass the filters entirely).
            // toneMode 1-3 → array index 0-2 (subtract 1).
            if (toneMode > 0)
            {
                const int modeIndex = toneMode - 1;

                // Three biquad filters in series:
                //   Band 0: low shelf
                //   Band 1: peak
                //   Band 2: high shelf
                // processSample() takes one float in, returns one float out,
                // and internally updates the filter's state memory.
                for (int band = 0; band < 3; ++band)
                    x = toneFilters[modeIndex][band][ch].processSample (x);
            }

            // Write the processed sample back to the buffer.
            channels[ch][sample] = x;
        }
    }
}

// ============================================================================
//  State Saving / Loading
//
//  The DAW calls getStateInformation() when the user saves their session.
//  It calls setStateInformation() when they reload it.
//  We serialize the APVTS (all parameters) to XML.
// ============================================================================
void TubePreampProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void TubePreampProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

// ============================================================================
//  Editor Creation
// ============================================================================
juce::AudioProcessorEditor* TubePreampProcessor::createEditor()
{
    return new TubePreampEditor (*this);
}

// ============================================================================
//  Plugin entry point — called by the host to create the plugin instance.
// ============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TubePreampProcessor();
}
