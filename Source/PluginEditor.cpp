#include "PluginProcessor.h"
#include "PluginEditor.h"

// ============================================================================
//  Constructor
//
//  Everything that sets up the GUI happens here:
//    1. Set window size
//    2. Configure knobs
//    3. Create parameter attachments (links knobs to APVTS parameters)
//    4. Configure tone buttons
//    5. Start the automation-sync timer
// ============================================================================
TubePreampEditor::TubePreampEditor (TubePreampProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // ── Window size ────────────────────────────────────────────────────────
    setSize (440, 280);
    setResizable (false, false);  // fixed size keeps layout simple

    // ── Drive Knob ─────────────────────────────────────────────────────────
    //
    // RotaryVerticalDrag = drag up to increase, drag down to decrease.
    // This is the standard "endless" knob feel used in most DAWs.
    //
    driveKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);

    // TextBoxBelow = value display underneath the knob
    // (false = not read-only, so user can click and type a value)
    driveKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    driveKnob.setNumDecimalPlacesToDisplay (2);

    // Double-click resets to center (0.0 = clean, no saturation).
    // This is a great UX feature for bipolar controls.
    driveKnob.setDoubleClickReturnValue (true, 0.0);

    // Colour the knob components
    driveKnob.setColour (juce::Slider::rotarySliderFillColourId,  colAccent);
    driveKnob.setColour (juce::Slider::rotarySliderOutlineColourId, colKnobBg);
    driveKnob.setColour (juce::Slider::thumbColourId,              colAccent);
    driveKnob.setColour (juce::Slider::textBoxTextColourId,        colTextDim);
    driveKnob.setColour (juce::Slider::textBoxBackgroundColourId,  juce::Colours::transparentBlack);
    driveKnob.setColour (juce::Slider::textBoxOutlineColourId,     juce::Colours::transparentBlack);

    addAndMakeVisible (driveKnob);

    // Label above the Drive knob
    driveLabel.setText ("DRIVE", juce::dontSendNotification);
    driveLabel.setJustificationType (juce::Justification::centred);
    driveLabel.setFont (juce::Font (11.0f, juce::Font::bold));
    driveLabel.setColour (juce::Label::textColourId, colTextDim);
    addAndMakeVisible (driveLabel);

    // ── Mic Level Knob ─────────────────────────────────────────────────────
    micGainKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    micGainKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    micGainKnob.setNumDecimalPlacesToDisplay (1);
    micGainKnob.setTextValueSuffix (" dB");        // append "dB" to the displayed value

    // Double-click resets to 0 dB (unity gain)
    micGainKnob.setDoubleClickReturnValue (true, 0.0);

    micGainKnob.setColour (juce::Slider::rotarySliderFillColourId,   colAccent);
    micGainKnob.setColour (juce::Slider::rotarySliderOutlineColourId, colKnobBg);
    micGainKnob.setColour (juce::Slider::thumbColourId,               colAccent);
    micGainKnob.setColour (juce::Slider::textBoxTextColourId,         colTextDim);
    micGainKnob.setColour (juce::Slider::textBoxBackgroundColourId,   juce::Colours::transparentBlack);
    micGainKnob.setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);

    addAndMakeVisible (micGainKnob);

    micGainLabel.setText ("MIC LEVEL", juce::dontSendNotification);
    micGainLabel.setJustificationType (juce::Justification::centred);
    micGainLabel.setFont (juce::Font (11.0f, juce::Font::bold));
    micGainLabel.setColour (juce::Label::textColourId, colTextDim);
    addAndMakeVisible (micGainLabel);

    // ── Parameter Attachments ─────────────────────────────────────────────
    //
    // SliderAttachment does the two-way binding between the knob and the APVTS:
    //   • User moves knob → calls audioProcessor.apvts to update the parameter
    //   • DAW automation moves the parameter → knob visually follows
    //
    // The attachment must be created AFTER the knob is configured, and must be
    // destroyed BEFORE the knob is destroyed. Using unique_ptr in the header
    // ensures correct destruction order.
    //
    driveAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "drive",    driveKnob);

    micGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "micGain",  micGainKnob);

    // ── Tone Mode Buttons ─────────────────────────────────────────────────
    //
    // We get a raw pointer to the toneMode parameter so we can:
    //   a) Read its current value (to initialize button states)
    //   b) Set its value when a button is clicked
    //
    // dynamic_cast<AudioParameterInt*> safely converts from the base class
    // AudioProcessorParameter* to the derived AudioParameterInt* type.
    // jassert() will throw a debug error if this returns nullptr (it won't,
    // since we know we defined it as AudioParameterInt in the Processor).
    //
    toneModeParam = dynamic_cast<juce::AudioParameterInt*>(
        audioProcessor.apvts.getParameter ("toneMode"));
    jassert (toneModeParam != nullptr);

    // A helper lambda to configure each button with the same style.
    // Lambda captures [this] to access member variables.
    auto setupButton = [this] (juce::TextButton& btn, int modeIndex)
    {
        // We manage toggle state ourselves, so disable JUCE's built-in toggle.
        btn.setClickingTogglesState (false);

        // When clicked: call onToneButtonClicked() with this button's mode index.
        // modeIndex: 1=SSL, 2=Neve, 3=Focusrite
        btn.onClick = [this, modeIndex]() { onToneButtonClicked (modeIndex); };

        // Set font for button text
        btn.setColour (juce::TextButton::textColourOffId, colTextDim);
        btn.setColour (juce::TextButton::textColourOnId,  colTextBright);

        addAndMakeVisible (btn);
    };

    setupButton (sslButton,   1);
    setupButton (neveButton,  2);
    setupButton (focusButton, 3);

    // Initialize button visual states from current parameter value.
    updateToneButtons ((int)toneModeParam->get());

    // ── Automation sync timer ─────────────────────────────────────────────
    // Start a 100ms repeating timer. On each tick, we check if the toneMode
    // parameter was changed by DAW automation and update the buttons if needed.
    startTimerHz (10);  // 10 Hz = every 100ms — plenty for visual feedback
}

TubePreampEditor::~TubePreampEditor()
{
    stopTimer();
}

// ============================================================================
//  onToneButtonClicked()
//
//  Called when one of the three tone buttons is clicked.
//  Logic: if the clicked button is already active, turn it off (set mode to 0).
//         If a different button is clicked, activate that mode.
// ============================================================================
void TubePreampEditor::onToneButtonClicked (int modeIndex)
{
    const int currentMode = (int)toneModeParam->get();

    // Toggle: clicking the active button turns it off.
    const int newMode = (currentMode == modeIndex) ? 0 : modeIndex;

    // Update the parameter using the gesture API.
    // beginChangeGesture / endChangeGesture tells the host we're making
    // a user-initiated change — needed for proper automation recording.
    toneModeParam->beginChangeGesture();
    toneModeParam->setValueNotifyingHost (
        toneModeParam->convertTo0to1 ((float)newMode));
    toneModeParam->endChangeGesture();

    // Update button colors immediately (no need to wait for the timer).
    updateToneButtons (newMode);
}

// ============================================================================
//  updateToneButtons()
//
//  Visually refreshes all three buttons to match the current active mode.
//  Call this whenever toneMode changes — whether from user click or automation.
// ============================================================================
void TubePreampEditor::updateToneButtons (int activeMode)
{
    // Helper lambda: set one button's colour based on whether it's active.
    auto applyColour = [this] (juce::TextButton& btn, bool isActive)
    {
        btn.setColour (juce::TextButton::buttonColourId,
                       isActive ? colButtonOn : colButtonOff);
        btn.setColour (juce::TextButton::textColourOffId,
                       isActive ? colTextBright : colTextDim);
    };

    // activeMode: 1=SSL, 2=Neve, 3=Focusrite, 0=none
    applyColour (sslButton,   activeMode == 1);
    applyColour (neveButton,  activeMode == 2);
    applyColour (focusButton, activeMode == 3);
}

// ============================================================================
//  timerCallback()
//
//  Called every 100ms. Polls the toneMode parameter to sync button visuals
//  when the DAW plays back automation on the tone mode parameter.
// ============================================================================
void TubePreampEditor::timerCallback()
{
    if (toneModeParam != nullptr)
        updateToneButtons ((int)toneModeParam->get());
}

// ============================================================================
//  paint()
//
//  Draws the plugin's static background graphics: background color, title text,
//  separator lines, and section labels.
//
//  JUCE redraws this whenever the component is invalidated (e.g., when the
//  window is resized or the host requests a repaint). All drawing is done with
//  the Graphics context object (g).
//
//  The knobs and buttons draw themselves — we don't draw those here.
// ============================================================================
void TubePreampEditor::paint (juce::Graphics& g)
{
    const int w = getWidth();

    // ── Background ─────────────────────────────────────────────────────────
    g.fillAll (colBackground);

    // ── Title ───────────────────────────────────────────────────────────────
    g.setColour (colAccent);
    g.setFont (juce::Font (22.0f, juce::Font::bold));
    g.drawFittedText ("TUBE PREAMP", 0, 10, w, 28,
                      juce::Justification::centred, 1);

    // ── Subtitle ────────────────────────────────────────────────────────────
    g.setColour (colTextDim);
    g.setFont (juce::Font (9.5f, juce::Font::plain));
    g.drawFittedText ("ANALOG CHARACTER EMULATOR", 0, 36, w, 16,
                      juce::Justification::centred, 1);

    // ── Separator line under title ──────────────────────────────────────────
    g.setColour (colSeparator);
    g.drawLine (20.0f, 56.0f, (float)w - 20.0f, 56.0f, 1.0f);

    // ── "CONSOLE CHARACTER" section label above the buttons ────────────────
    g.setColour (colTextDim);
    g.setFont (juce::Font (9.0f, juce::Font::plain));
    g.drawFittedText ("CONSOLE CHARACTER", 0, 202, w, 14,
                      juce::Justification::centred, 1);

    // ── Drive bipolar indicator ─────────────────────────────────────────────
    // Small labels to indicate the bipolar nature of the Drive knob.
    // "CLEAN" on the left side, "HOT" on the right side.
    const int knobSize       = 100;
    const int spacing        = 40;
    const int totalKnobWidth = knobSize * 2 + spacing;
    const int knobAreaLeft   = (w - totalKnobWidth) / 2;
    const int knobTop        = 62;
    const int labelH         = 20;

    g.setFont (juce::Font (8.5f));
    g.setColour (colTextDim.withAlpha (0.7f));

    // Position "CLEAN" and "HOT" under the drive knob text box
    const int driveCenter = knobAreaLeft + knobSize / 2;
    const int indicatorY  = knobTop + labelH + knobSize + 20;  // below textbox
    g.drawFittedText ("CLEAN",  driveCenter - 50, indicatorY, 45, 12,
                      juce::Justification::centred, 1);
    g.drawFittedText ("HOT",    driveCenter + 5,  indicatorY, 45, 12,
                      juce::Justification::centred, 1);
}

// ============================================================================
//  resized()
//
//  Called whenever the window size changes (and once after construction).
//  This is where we position all child components (knobs, labels, buttons).
//
//  We calculate positions mathematically so the layout stays centered
//  and proportional. No hardcoded magic numbers where possible.
// ============================================================================
void TubePreampEditor::resized()
{
    const int w = getWidth();

    // ── Knob layout math ─────────────────────────────────────────────────
    const int knobSize  = 100;   // knob diameter in pixels
    const int spacing   = 40;    // gap between the two knobs
    const int knobTop   = 62;    // top of the label area
    const int labelH    = 18;    // height of label text above each knob

    // Center the two-knob group horizontally
    const int totalKnobWidth = knobSize * 2 + spacing;
    const int knobAreaLeft   = (w - totalKnobWidth) / 2;

    // ── Drive knob (left) ─────────────────────────────────────────────────
    driveLabel.setBounds (knobAreaLeft,            knobTop,          knobSize, labelH);
    driveKnob .setBounds (knobAreaLeft,            knobTop + labelH, knobSize, knobSize);

    // ── Mic Level knob (right) ────────────────────────────────────────────
    const int micLeft = knobAreaLeft + knobSize + spacing;
    micGainLabel.setBounds (micLeft,  knobTop,          knobSize, labelH);
    micGainKnob .setBounds (micLeft,  knobTop + labelH, knobSize, knobSize);

    // ── Tone mode buttons ─────────────────────────────────────────────────
    const int btnW       = 114;   // button width
    const int btnH       = 30;    // button height
    const int btnY       = 218;   // vertical position
    const int btnGap     = 12;    // gap between buttons

    // Center the three-button group horizontally
    const int totalBtnW  = btnW * 3 + btnGap * 2;
    const int btnLeft    = (w - totalBtnW) / 2;

    sslButton  .setBounds (btnLeft,                      btnY, btnW, btnH);
    neveButton .setBounds (btnLeft + (btnW + btnGap),    btnY, btnW, btnH);
    focusButton.setBounds (btnLeft + (btnW + btnGap) * 2, btnY, btnW, btnH);
}
