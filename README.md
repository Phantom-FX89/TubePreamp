# Tube Preamp

Stereo macOS insert effect (AU and VST3): a wide gain stage, a bipolar waveshaper, and an optional fixed three-band EQ.

The window subtitle is "ANALOG CHARACTER EMULATOR". The DSP below is the whole product. Button names are labels on those EQ curves.

## Behavior

Path inside `processBlock`:

```
input → mic gain → drive → tone EQ (or bypass) → output
```

Default buses are stereo in and stereo out. There is no MIDI, no sidechain bus, and no tail. `processBlock` writes the first two channels it is given and leaves any further channels alone. The processor does not override `isBusesLayoutSupported`.

One program slot. `getProgramName` returns an empty string. There is no preset browser.

The host stores the three parameters as an `AudioProcessorValueTreeState` XML blob (`TubePreampState`). On load, state is applied only when that tag matches.

| Host parameter | ID | Range | Default | Smoothing |
| --- | --- | --- | --- | --- |
| Mic Gain | `micGain` | -20 dB to +60 dB, 0.1 dB steps | 0 dB | 20 ms |
| Drive | `drive` | -1 to +1, 0.01 steps | 0 | 20 ms |
| Tone Mode | `toneMode` | 0 off, 1 SSL 4K, 2 Neve 1073, 3 Focusrite | 0 (off) | none (read once per block) |

Gain is converted with `juce::Decibels::decibelsToGain` before it multiplies the sample. Tone mode is not smoothed, so a switch can click. The editor polls `toneMode` at 10 Hz so the buttons follow host automation.

Samples are not clipped after the chain.

### Mic gain

Unity is 0 dB. Negative values pad a hot signal down. Positive values boost it, up to +60 dB (a linear gain of 1000). Double-click the knob to return to 0 dB. The value box shows one decimal and a `dB` suffix, and it accepts typed values.

### Drive

Absolute drive below 0.001 leaves the sample unchanged. Double-click returns to 0. The value box shows two decimals. The GUI paints "CLEAN" on the left of the knob and "HOT" on the right. The passthrough is the center.

Positive drive (`drive > 0`):

```
k = 1 + drive * 11          // about 1 at just above 0, 12 at +1
y = tanh(k * x) / tanh(k)
```

`tanh` is odd, so this path produces odd-order harmonics. Dividing by `tanh(k)` holds a full-scale input (±1) at full scale. Larger inputs, including a hot mic-gain stage, squash toward ±1. There is no oversampling, so those harmonics can alias.

Negative drive (`drive < 0`):

```
amount = (-drive) * 0.15    // 0.15 at drive = -1
sign   = +1 if x > 0 else -1
y      = x + amount * x * x * sign
```

That expression is also odd (`f(-x) = -f(x)`), so a sine yields odd-order harmonics. It has no ceiling. A full-scale sample at +60 dB is a linear gain of 1000; at drive = -1 that sample becomes about 151000. Comments in `Source/PluginProcessor.cpp` and `.agent/ARCHITECTURE.md` call this path even-harmonic "Class A" warmth. The formula above is what runs.

### Tone EQ

Three buttons: **SSL 4K**, **NEVE 1073**, **FOCUSRITE**. They select `toneMode` 1, 2, or 3. Click the active button again and the mode returns to 0 (filters skipped).

Each mode is three RBJ biquads in series (low shelf, peak, high shelf), from `juce::dsp::IIR::Coefficients`, stereo, coefficients set in `prepareToPlay` from the sample rate. Gains are not automatable. The repo does not include the measurements or a circuit model behind the names.

| Mode | Low shelf | Peak | High shelf |
| --- | --- | --- | --- |
| SSL 4K | +1.5 dB at 80 Hz, Q 0.71 | +2.0 dB at 3.5 kHz, Q 0.80 | +1.5 dB at 12 kHz, Q 0.71 |
| Neve 1073 | +3.0 dB at 60 Hz, Q 0.71 | +2.5 dB at 3.2 kHz, Q 0.70 | +1.0 dB at 10 kHz, Q 0.71 |
| Focusrite | +1.5 dB at 80 Hz, Q 0.71 | +1.5 dB at 5 kHz, Q 1.20 | +2.5 dB at 16 kHz, Q 0.71 |

### Editor

`Source/PluginEditor.cpp` opens a fixed 440×280 window (`setResizable(false, false)`). Background `#1A1A1A`, accent `#E8A230`. Knobs are `RotaryVerticalDrag`, bound with `SliderAttachment`.

### What this build does not contain

No output trim, no oversampling, no spectrum, no custom knob graphics, no factory presets, no MIDI, no standalone app, no VST2, no AAX. `CMakeLists.txt` mentions AAX only in a comment. `FORMATS` is `AU VST3`.

## Requirements

Build and install are macOS-only. `install.sh` uses `xcode-select`, `sysctl`, `lipo`, and `~/Library`.

| Requirement | Where it is set |
| --- | --- |
| CMake 3.22 or newer | `cmake_minimum_required` in `CMakeLists.txt` |
| C++17 | `CMAKE_CXX_STANDARD` |
| macOS deployment target 11.0 | `CMAKE_OSX_DEPLOYMENT_TARGET` |
| Universal binary `arm64` and `x86_64` | `CMAKE_OSX_ARCHITECTURES` |
| A selected developer directory | `install.sh` requires `xcode-select -p` to succeed. The script comment asks for full Xcode. The check itself also passes for the Command Line Tools. |
| Git | JUCE is a submodule |

CMake is not bundled. The script's error text suggests `brew install cmake`. Any `cmake` 3.22+ on `PATH` satisfies the check. The script does not check an Xcode version.

## Setup

`.gitmodules` already points `JUCE` at `https://github.com/juce-framework/JUCE.git`, pinned to commit [`501c07674e1ad693085a7e7c398f205c2677f5da`](https://github.com/juce-framework/JUCE/commit/501c07674e1ad693085a7e7c398f205c2677f5da) (JUCE module version 8.0.12 in `juce_core`).

From a clone:

```bash
git submodule update --init --recursive
```

Do not run `git init` or `git submodule add` for JUCE. The submodule is already recorded.

`install.sh` is executable in git (`100755`). If a checkout drops that bit:

```bash
chmod +x install.sh
```

`install.sh` treats any `JUCE/` directory as success (`[ -d JUCE ]`). An empty directory passes, and CMake then fails in `add_subdirectory(JUCE)` because `JUCE/CMakeLists.txt` is missing. Initialize the submodule before configuring.

If the `JUCE/` directory is actually absent, the script exits and prints `git init` plus `git submodule add`. That message does not match this repo. Use the submodule command above.

Plugin identity is already set. These values are the AU/VST3 identity hosts store. Change them only to ship a different plugin.

| Field | Value |
| --- | --- |
| `COMPANY_NAME` | Phantom House Audio |
| `PLUGIN_MANUFACTURER_CODE` | `P747` |
| `PLUGIN_CODE` | `T740` |
| `BUNDLE_ID` | `com.phantomhouseaudio.tubepreamp` |
| `PRODUCT_NAME` | Tube Preamp |
| Version | 1.0.0 |
| Formats | AU, VST3 |
| VST3 categories | `Fx`, `Dynamics` |

`Dynamics` is only the VST3 category string. The processor is not a compressor or a gate.

## Build and install

Run from the repository root on a Mac:

```bash
./install.sh
```

What the script does:

1. Confirms `CMakeLists.txt`, `xcode-select -p`, `cmake`, and a `JUCE/` directory.
2. If `build/` is missing, configures with the same architecture and deployment flags as `CMakeLists.txt`:

   ```bash
   cmake -B build \
     -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
     -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0"
   ```

   It does not pass `-G Xcode`. CMake uses its default generator (Unix Makefiles, unless `CMAKE_GENERATOR` is already set).
3. If `build/` already exists, configure is skipped. `./install.sh --clean` deletes `build/` and configures again. `--clean` is the only flag.
4. Builds Release, parallel across `sysctl -n hw.ncpu`. `set -e` stops the script on the first failing command.
5. Copies `Tube Preamp.component` to `~/Library/Audio/Plug-Ins/Components/`.
6. Copies `Tube Preamp.vst3` to `~/Library/Audio/Plug-Ins/VST3/`.
7. Runs `killall -9 AudioComponentRegistrar` (failures ignored) so hosts drop the cached AU scan.

The script does not create those Library folders. If `cp` fails because a folder is missing:

```bash
mkdir -p ~/Library/Audio/Plug-Ins/Components ~/Library/Audio/Plug-Ins/VST3
```

Default-generator artifacts:

- `build/TubePreamp_artefacts/AU/Tube Preamp.component`
- `build/TubePreamp_artefacts/VST3/Tube Preamp.vst3`

If the Xcode generator was used, the script also looks under `build/TubePreamp_artefacts/Release/`. When the VST3 binary exists, it prints `lipo -info`. It does not fail the install when the binary is single-architecture.

Configure and build without installing:

```bash
git submodule update --init --recursive
cmake -B build \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0"
cmake --build build --config Release --parallel
```

Linked JUCE modules: `juce_audio_utils`, `juce_audio_processors`, `juce_dsp`, `juce_gui_basics`, `juce_gui_extra`, `juce_core`. Public flags: `juce_recommended_config_flags`, `juce_recommended_lto_flags`, `juce_recommended_warning_flags`.

Compile definitions: `JUCE_WEB_BROWSER=0`, `JUCE_USE_CURL=0`, `JUCE_VST3_CAN_REPLACE_VST2=0`, `JUCE_DISPLAY_SPLASH_SCREEN=0`. The splash flag is ignored by this JUCE pin; see License.

After a successful install, the script prints this rescan list:

- Logic Pro: quit and relaunch (AU scan on launch).
- Ableton Live: Preferences → Plug-Ins → Rescan.
- Reaper: Options → Preferences → VST → Rescan.
- Studio One: Studio One menu → Options → Locations → Scan.

## Configuration

`Source/` and `install.sh` do not read environment variables, a dotenv file, or any other config file. There is no `.env.example` in the tree. Do not commit secrets; nothing in this repo needs one.

`AAX_SDK_PATH` appears in a comment in `CMakeLists.txt`. No CMake or shell code reads it.

`.gitignore` on `develop` ignores macOS Finder files, `/build/`, `/JuceLibraryCode/`, Xcode projects and user files (`*.xcode*`, `*.xcodeproj`, `*.xcworkspace`, `*.user`), `*.bak`, `*.old`, and `.agent/task.md`.

[Hygiene PR #2](https://github.com/Phantom-FX89/TubePreamp/pull/2) adds `.env`, `.env.*` (with `!.env.example`), `*.log`, and `cmake-build-*/`. Those lines are not on `develop` until that PR merges. This change does not edit `.gitignore`.

## Project structure

Checked against the tree. There is no `docs/` directory, no `tests/` directory, no `.github/` workflow, no `LICENSE`, and no `.env.example`. `JUCE/` is empty until the submodule is initialized.

```
.
├── .agent/                         internal notes; the build does not read these
│   ├── ACCEPTANCE.md
│   ├── ARCHITECTURE.md
│   ├── MISSION.md
│   ├── NOTES.md
│   ├── RUNBOOK.md
│   └── workflows/
│       ├── analyze.md
│       ├── build.md
│       ├── credit-saver.md
│       ├── design.md
│       ├── fix.md
│       ├── global-workflows.md
│       ├── kickstart.md
│       ├── ops.md
│       ├── refactor.md
│       ├── ship.md
│       ├── spec.md
│       ├── test-me.md
│       ├── test.md
│       └── universal.md
├── .gitignore
├── .gitmodules                     JUCE → https://github.com/juce-framework/JUCE.git
├── CMakeLists.txt
├── install.sh
├── README.md
├── JUCE/                           submodule, commit 501c076
└── Source/
    ├── PluginEditor.cpp
    ├── PluginEditor.h
    ├── PluginProcessor.cpp
    └── PluginProcessor.h
```

`.agent/RUNBOOK.md` and `.agent/NOTES.md` are empty templates. `.agent/ACCEPTANCE.md` is an unchecked manual checklist.

## Usage

1. Initialize the submodule and run `./install.sh` on a Mac (see above).
2. Rescan plugins in the host.
3. Insert **Tube Preamp** on a track. AU hosts see manufacturer `P747`, subtype `T740`, type `aufx`. VST3 hosts see "Tube Preamp" from Phantom House Audio.
4. Set mic gain for level, drive for the waveshaper, and one tone button or none.

Drive in the dead zone (`|drive| < 0.001`) returns the sample unchanged. 0 dB gain is unity. A tone button adds that mode's fixed curve. High mic gain with positive drive squashes toward full scale. High mic gain with negative drive is unbounded.

Parameter moves on `micGain` and `drive` are recorded as host automation. `toneMode` is an integer parameter; the buttons record a gesture via `beginChangeGesture` / `endChangeGesture`. Reload the DAW session to restore the XML state.

## Testing

This repository has no test sources, no CTest target, and no GitHub Actions workflow. There is no CI badge for that reason.

The build check that exists is a Mac run of `./install.sh`.

Optional Audio Unit validation after install, using the type and codes from `CMakeLists.txt` and JUCE's effect mapping (`kAudioUnitType_Effect` → `aufx`):

```bash
auval -v aufx T740 P747
```

`.agent/ACCEPTANCE.md` lists that `auval` check, a DAW load, and the install paths. Nothing in the repo runs the list.

## Contributing

Pull requests target `develop`.

Keep `P747` / `T740` stable unless the plugin identity change is the point of the change. Update JUCE by changing the submodule pin, not by copying the framework into the tree. `./install.sh` is the macOS build path; `CMakeLists.txt` is the source of architectures, formats, and deployment target.

`.agent/` is workflow notes. The processor is the spec when those notes disagree with it.

Licensing is an owner decision. Do not add or rewrite `LICENSE` in a feature PR. See below.

## License

`develop` has no `LICENSE` file, and the source files have no license header. [PR #1](https://github.com/Phantom-FX89/TubePreamp/pull/1) proposes adding an MIT `LICENSE`. This README does not adopt that text. Merge of that PR is up to the owner.

JUCE is not covered by that proposal. The pinned submodule is dual-licensed under the [AGPLv3](https://www.gnu.org/licenses/agpl-3.0.en.html) or the [JUCE 8 commercial licence](https://juce.com/legal/juce-8-licence/). Terms are in the submodule's `LICENSE.md` after init. This repository does not grant a JUCE licence.

`CMakeLists.txt` sets `JUCE_DISPLAY_SPLASH_SCREEN=0`. The comment on that line says the splash requires a JUCE licence. On the pinned JUCE 8.0.12, `modules/juce_gui_basics/juce_gui_basics.cpp` does not draw a splash screen. If `JUCE_DISPLAY_SPLASH_SCREEN` is defined at all, that file emits a compiler warning that the flag is ignored. The define does not choose a licence.
