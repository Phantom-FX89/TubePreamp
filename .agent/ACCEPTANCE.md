# ACCEPTANCE

**Prerequisites Check:**
- [ ] macOS environment (Big Sur or newer) is present.
- [ ] Xcode / command-line tools are installed.
- [ ] CMake (3.22+) is installed.
- [ ] JUCE submodule is present and initialized.

**Build & Installation:**
- [ ] `./install.sh` runs successfully without terminal errors.
- [ ] The Universal Binary (arm64 + x86_64) builds successfully.
- [ ] AU component is installed to `~/Library/Audio/Plug-Ins/Components/`.
- [ ] VST3 component is installed to `~/Library/Audio/Plug-Ins/VST3/`.

**Functional Validation (Baseline):**
- [ ] Audio unit passes `auval` tool validation.
- [ ] Plugin loads in a DAW (e.g., Reaper, Ableton, Logic).
- [ ] Audio processes correctly through the Tube Saturation and EQ stages without artifacts.
