#!/bin/bash
# =============================================================================
#  TubePreamp — Build & Install Script
#  Apple Silicon (arm64 + x86_64 Universal Binary)
#
#  Usage:
#    chmod +x install.sh     (first time only — makes the script executable)
#    ./install.sh            (builds and installs to your DAW plugin folders)
#    ./install.sh --clean    (deletes the build folder first, then rebuilds)
# =============================================================================

set -e  # exit immediately if any command fails

PLUGIN_NAME="TubePreamp"
BUNDLE_NAME="Tube Preamp"
BUILD_DIR="build"

# ── Colour output helpers ────────────────────────────────────────────────────
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Colour

ok()   { echo -e "${GREEN}  ✅ $1${NC}"; }
warn() { echo -e "${YELLOW}  ⚠️  $1${NC}"; }
err()  { echo -e "${RED}  ❌ $1${NC}"; exit 1; }

echo ""
echo "=================================================="
echo "  Tube Preamp — Build & Install"
echo "=================================================="
echo ""

# ── Handle --clean flag ──────────────────────────────────────────────────────
if [[ "$1" == "--clean" ]]; then
    echo "Cleaning build directory..."
    rm -rf "${BUILD_DIR}"
    ok "Build directory removed"
    echo ""
fi

# ── Pre-build validation ─────────────────────────────────────────────────────
echo "Checking environment..."

# Must be run from the project root (where CMakeLists.txt lives)
[ -f CMakeLists.txt ] \
    && ok "CMakeLists.txt found" \
    || err "CMakeLists.txt not found — run this script from the TubePreamp/ directory"

# Xcode must be installed (not just Command Line Tools)
xcode-select -p &>/dev/null \
    && ok "Xcode: $(xcode-select -p)" \
    || err "Xcode not installed — download from the Mac App Store"

# CMake must be available
cmake --version &>/dev/null \
    && ok "CMake: $(cmake --version | head -1)" \
    || err "CMake not found — install with: brew install cmake"

# JUCE submodule must be present
[ -d "JUCE" ] \
    && ok "JUCE directory found" \
    || err "JUCE/ directory not found.
       Run these commands to add it:
         git init (if not already a git repo)
         git submodule add https://github.com/juce-framework/JUCE.git JUCE
         git submodule update --init --recursive"

echo ""

# ── Configure ────────────────────────────────────────────────────────────────
if [ ! -d "${BUILD_DIR}" ]; then
    echo "Configuring CMake..."
    cmake -B "${BUILD_DIR}" \
        -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0"
    ok "CMake configured"
else
    echo "Build directory exists — skipping configure (run with --clean to reconfigure)"
fi

echo ""

# ── Build ─────────────────────────────────────────────────────────────────────
echo "Building ${PLUGIN_NAME} (Release)..."
echo "Using $(sysctl -n hw.ncpu) CPU cores..."
echo ""

cmake --build "${BUILD_DIR}" \
    --config Release \
    --parallel $(sysctl -n hw.ncpu)

echo ""
ok "Build succeeded"
echo ""

# ── Verify Universal Binary ───────────────────────────────────────────────────
# With Unix Makefiles/Ninja, artifacts are directly in the artefacts dir (no Release/ folder)
# With Xcode, they are in Release/. Let's check both just in case.
ARTEFACTS="${BUILD_DIR}/${PLUGIN_NAME}_artefacts"
if [ ! -d "${ARTEFACTS}/AU" ] && [ -d "${ARTEFACTS}/Release/AU" ]; then
    ARTEFACTS="${ARTEFACTS}/Release"
fi

if [ -d "${ARTEFACTS}/VST3/${BUNDLE_NAME}.vst3" ]; then
    ARCHS=$(lipo -info "${ARTEFACTS}/VST3/${BUNDLE_NAME}.vst3/Contents/MacOS/${BUNDLE_NAME}" 2>/dev/null || echo "unknown")
    echo "Architecture check: ${ARCHS}"
fi

echo ""

# ── Install ───────────────────────────────────────────────────────────────────
echo "Installing plugins..."

# AU (Audio Unit) → Logic Pro, GarageBand, MainStage
AU_SRC="${ARTEFACTS}/AU/${BUNDLE_NAME}.component"
AU_DEST=~/Library/Audio/Plug-Ins/Components/
if [ -d "${AU_SRC}" ]; then
    # Remove old version if present
    rm -rf "${AU_DEST}${BUNDLE_NAME}.component"
    cp -r "${AU_SRC}" "${AU_DEST}"
    ok "AU installed → ${AU_DEST}"
else
    warn "AU artifact not found at ${AU_SRC} — skipping"
fi

# VST3 → Ableton, Reaper, Studio One, Bitwig, Cubase, etc.
VST3_SRC="${ARTEFACTS}/VST3/${BUNDLE_NAME}.vst3"
VST3_DEST=~/Library/Audio/Plug-Ins/VST3/
if [ -d "${VST3_SRC}" ]; then
    rm -rf "${VST3_DEST}${BUNDLE_NAME}.vst3"
    cp -r "${VST3_SRC}" "${VST3_DEST}"
    ok "VST3 installed → ${VST3_DEST}"
else
    warn "VST3 artifact not found at ${VST3_SRC} — skipping"
fi

echo ""

# ── Refresh audio component registry ─────────────────────────────────────────
# This kills and restarts the macOS daemon that caches plugin metadata.
# Without this, DAWs won't see the newly installed plugin until you log out.
echo "Refreshing audio component registry..."
killall -9 AudioComponentRegistrar 2>/dev/null || true
ok "Audio registry refreshed"

echo ""
echo "=================================================="
echo -e "${GREEN}  ✨ Done! Launch your DAW to use the plugin.${NC}"
echo ""
echo "  DAW-specific rescan:"
echo "    Logic Pro   → Quit and relaunch (AU auto-scans)"
echo "    Ableton     → Preferences → Plug-Ins → Rescan"
echo "    Reaper      → Options → Preferences → VST → Rescan"
echo "    Studio One  → Studio One menu → Options → Locations → Scan"
echo "=================================================="
echo ""
