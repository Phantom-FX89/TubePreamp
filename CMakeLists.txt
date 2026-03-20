cmake_minimum_required(VERSION 3.22)
project(TubePreamp VERSION 1.0.0)

# ── C++ standard ──────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── macOS targets ─────────────────────────────────────────────────────────────
# Universal Binary = runs natively on both Apple Silicon (arm64) AND Intel (x86_64).
# Always build this — even if you're on M1, your users may be on Intel.
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")

# Minimum macOS version. Big Sur (11.0) is the oldest that runs on Apple Silicon.
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")

# ── JUCE ──────────────────────────────────────────────────────────────────────
# OPTION A (recommended): JUCE as a git submodule in a "JUCE/" folder.
#   To set this up once in your project root:
#     git submodule add https://github.com/juce-framework/JUCE.git JUCE
#     git submodule update --init --recursive
add_subdirectory(JUCE)

# OPTION B: If you installed JUCE globally (e.g. via Homebrew), comment out the
#   add_subdirectory above and uncomment the find_package line below instead:
# find_package(JUCE CONFIG REQUIRED)

# ── Plugin definition ─────────────────────────────────────────────────────────
# juce_add_plugin() is JUCE's way to declare a plugin target. It handles all the
# format-specific boilerplate (Info.plist, AU factory, VST3 entry points, etc.)
juce_add_plugin(TubePreamp

    # ── Identity — CHANGE THESE to your own values ────────────────────────
    COMPANY_NAME                "Phantom House Audio"        # Your name or company name
    PLUGIN_MANUFACTURER_CODE    "P747"              # Exactly 4 ASCII chars, must be GLOBALLY unique
    PLUGIN_CODE                 "T740"              # Exactly 4 ASCII chars, must be unique per company
    BUNDLE_ID                   "com.phantomhouseaudio.tubepreamp"  # Reverse-domain ID

    # ── Plugin formats ─────────────────────────────────────────────────────
    # AU  = Logic Pro, GarageBand, MainStage
    # VST3 = Ableton Live, Reaper, Bitwig, Studio One, Cubase, etc.
    # To add AAX (Pro Tools) later: add AAX here and set AAX_SDK_PATH in env
    FORMATS                     AU VST3

    # ── Metadata ───────────────────────────────────────────────────────────
    PRODUCT_NAME                "Tube Preamp"
    VST3_CATEGORIES             "Fx" "Dynamics"
    AU_EXPORT_PREFIX            "TubePreamp"

    # ── Plugin behavior flags ──────────────────────────────────────────────
    IS_SYNTH                    FALSE   # This is an effect, not an instrument
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE   # Our knobs don't need keyboard input
)

# ── Generate JuceHeader.h ─────────────────────────────────────────────────────
juce_generate_juce_header(TubePreamp)

# ── Source files ──────────────────────────────────────────────────────────────
target_sources(TubePreamp PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

# ── JUCE modules ──────────────────────────────────────────────────────────────
# These are JUCE's pre-built libraries. Link only what you need to keep
# compile times fast.
target_link_libraries(TubePreamp
    PRIVATE
        juce::juce_audio_utils       # AudioProcessor, AudioProcessorEditor
        juce::juce_audio_processors  # Plugin wrapper infrastructure
        juce::juce_dsp               # IIR filters, SmoothedValue, etc.
        juce::juce_gui_basics        # Slider, TextButton, Label, Graphics
        juce::juce_gui_extra         # Extra GUI helpers
        juce::juce_core              # String, Memory, Math
    PUBLIC
        juce::juce_recommended_config_flags   # Sensible compiler optimizations
        juce::juce_recommended_lto_flags      # Link-time optimization (smaller, faster)
        juce::juce_recommended_warning_flags  # Strict warnings catch bugs early
)

# ── Compile definitions ───────────────────────────────────────────────────────
target_compile_definitions(TubePreamp PUBLIC
    JUCE_WEB_BROWSER=0               # We don't use web features (keeps binary small)
    JUCE_USE_CURL=0                  # We don't use network features
    JUCE_VST3_CAN_REPLACE_VST2=0     # Don't try to replace legacy VST2 slots
    JUCE_DISPLAY_SPLASH_SCREEN=0     # No JUCE splash screen (requires JUCE license)
)
