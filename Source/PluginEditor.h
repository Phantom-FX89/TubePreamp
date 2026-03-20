#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// ============================================================================
//  TubePreampEditor
//
//  This class IS the plugin's graphical user interface (GUI).
//  The host creates one instance of this when the user opens the plugin window.
//  It can be destroyed and recreated (the user might close and reopen the
//  plugin window), but the Processor (the audio engine) keeps running.
//
//  IMPORTANT THREADING NOTE:
//    The Editor runs on the UI/message thread.
//    The Processor runs on the audio thread.
//    They communicate through the APVTS — thread-safe parameter objects.
//    NEVER call audio processing code from the Editor, and NEVER do GUI
//    updates directly from processBlock().
//
//  Layout (440 x 280 pixels):
//
//    ┌──────────────────────────────────────────────┐
//    │           TUBE PREAMP                        │  Title
//    │        ANALOG CHARACTER EMULATOR             │  Subtitle
//    │──────────────────────────────────────────────│
//    │                                              │
//    │      DRIVE          MIC LEVEL                │  Labels
//    │     (  ◉  )         (  ◉  )                  │  Knobs
//    │      0.00           0.0 dB                   │  Value displays
//    │                                              │
//    │          CONSOLE CHARACTER                   │  Section label
//    │   [ SSL 4K ]  [ NEVE 1073 ]  [FOCUSRITE]     │  Tone buttons
//    └──────────────────────────────────────────────┘
//
// ============================================================================
class TubePreampEditor  :  public juce::AudioProcessorEditor,
                            private juce::Timer            // for polling toneMode automation
{
public:
    TubePreampEditor (TubePreampProcessor&);
    ~TubePreampEditor() override;

    // JUCE GUI overrides
    void paint   (juce::Graphics&) override;  // background + labels + decorations
    void resized () override;                  // positions all child components

private:
    // Reference back to the processor — we access apvts through it.
    TubePreampProcessor& audioProcessor;

    // ── Knobs ─────────────────────────────────────────────────────────────
    // juce::Slider with RotaryVerticalDrag style = a knob you drag up/down
    juce::Slider driveKnob;
    juce::Slider micGainKnob;

    // Labels displayed above the knobs
    juce::Label driveLabel;
    juce::Label micGainLabel;

    // ── APVTS Slider Attachments ───────────────────────────────────────────
    // An attachment links a Slider to an APVTS parameter, so:
    //   • Moving the slider → updates the parameter (UI → audio thread)
    //   • DAW automation playback → moves the slider visually (audio → UI)
    // std::unique_ptr because attachments must outlive the slider they're attached to,
    // and we don't know the lifetime at compile time.
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> micGainAttachment;

    // ── Tone Mode Buttons ──────────────────────────────────────────────────
    // Three buttons that act as a mutually-exclusive selector.
    // We manage their toggle state ourselves rather than using JUCE's
    // built-in toggle, so we can link them to an integer parameter.
    juce::TextButton sslButton    { "SSL 4K" };
    juce::TextButton neveButton   { "NEVE 1073" };
    juce::TextButton focusButton  { "FOCUSRITE" };

    // Direct pointer to the toneMode parameter for reading and writing.
    // Dynamic_cast<AudioParameterInt*> is safe here — we know the type.
    juce::AudioParameterInt* toneModeParam = nullptr;

    // ── Tone button helpers ────────────────────────────────────────────────
    // Called when a tone button is clicked.
    // modeIndex: 1=SSL, 2=Neve, 3=Focusrite
    void onToneButtonClicked (int modeIndex);

    // Refreshes button visual state to match the current parameter value.
    // (Also called by the Timer to sync with DAW automation)
    void updateToneButtons (int activeMode);

    // ── Timer ──────────────────────────────────────────────────────────────
    // We poll the toneMode parameter every 100ms to keep the buttons in
    // sync when the DAW plays back automation on that parameter.
    void timerCallback() override;

    // ── Colour palette (defined once, used throughout paint() and resized()) ─
    const juce::Colour colBackground  { 0xFF1A1A1A };  // Very dark grey
    const juce::Colour colAccent      { 0xFFE8A230 };  // Warm amber (tube glow)
    const juce::Colour colKnobBg      { 0xFF2A2A2A };  // Dark knob surround
    const juce::Colour colButtonOn    { 0xFFE8A230 };  // Active button: amber
    const juce::Colour colButtonOff   { 0xFF2A2A2A };  // Inactive button: dark
    const juce::Colour colTextBright  { 0xFFFFFFFF };  // White text
    const juce::Colour colTextDim     { 0xFF888888 };  // Grey text
    const juce::Colour colSeparator   { 0xFF333333 };  // Separator line

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TubePreampEditor)
};
