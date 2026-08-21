#!/bin/bash
#
# CasparCG macOS Packaging Script
#
# Creates a signed, notarized macOS .app bundle + DMG from the build output.
# By default everything is enabled (NDI, signing, notarization, DMG).
# Signing credentials are read from .env file in the project root.
#
# Usage:
#   ./package.sh                              # Full: signed, notarized DMG with NDI
#   ./package.sh --no-sign                    # Unsigned app bundle + DMG
#   ./package.sh --no-notarize                # Signed but not notarized
#   ./package.sh --no-dmg --no-sign           # Quick: unsigned app bundle only
#
# Setup (.env file):
#   SIGNING_IDENTITY="Developer ID Application: Your Name (TEAMID)"
#   NOTARIZE_KEYCHAIN_PROFILE="CasparCG-Notarize"
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$ROOT_DIR"

# Load .env file if it exists
if [ -f "$ROOT_DIR/.env" ]; then
    echo "Loading configuration from .env file..."
    # Export variables from .env (ignore comments and empty lines)
    set -a
    source "$ROOT_DIR/.env"
    set +a
fi

# Configuration
APP_NAME="CasparCG"
BUNDLE_ID="com.casparcg.server"
BUILD_DIR="build"
OUTPUT_DIR="dist"

# Parse version from CMakeLists.txt
VERSION_MAJOR=$(grep "CONFIG_VERSION_MAJOR" src/CMakeLists.txt | head -1 | sed 's/.*CONFIG_VERSION_MAJOR \([0-9]*\).*/\1/')
VERSION_MINOR=$(grep "CONFIG_VERSION_MINOR" src/CMakeLists.txt | head -1 | sed 's/.*CONFIG_VERSION_MINOR \([0-9]*\).*/\1/')
VERSION_BUG=$(grep "CONFIG_VERSION_BUG" src/CMakeLists.txt | head -1 | sed 's/.*CONFIG_VERSION_BUG \([0-9]*\).*/\1/')
VERSION="${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_BUG}"

# Get git hash
if [ -d ".git" ]; then
    GIT_HASH=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
else
    GIT_HASH="unknown"
fi

# Options - everything enabled by default
# Use .env file for signing credentials (SIGNING_IDENTITY, NOTARIZE_KEYCHAIN_PROFILE)
# Use --no-* flags to disable features
INCLUDE_NDI=true
CREATE_DMG=true
SIGN_APP=true
NOTARIZE=true
OPT_SIGNING_IDENTITY="${SIGNING_IDENTITY:-}"
OPT_KEYCHAIN_PROFILE="${NOTARIZE_KEYCHAIN_PROFILE:-}"
NDI_LIB_PATH=""

# Print usage
usage() {
    echo "Usage: $0 [options]"
    echo ""
    echo "By default, builds a signed, notarized DMG with NDI included."
    echo "Signing credentials are read from .env file (SIGNING_IDENTITY, NOTARIZE_KEYCHAIN_PROFILE)."
    echo ""
    echo "Options:"
    echo "  --no-ndi                 Exclude NDI library"
    echo "  --ndi-path [path]        Specify custom path to libndi.dylib"
    echo "  --no-dmg                 Skip DMG creation (app bundle only)"
    echo "  --no-sign                Skip code signing (ad-hoc sign only)"
    echo "  --no-notarize            Skip notarization"
    echo "  --identity \"...\"         Override signing identity from .env"
    echo "  --keychain-profile \"...\" Override keychain profile from .env"
    echo "  -h, --help               Show this help"
    echo ""
    echo "Setup (.env file in project root):"
    echo "  SIGNING_IDENTITY=\"Developer ID Application: Your Name (TEAMID)\""
    echo "  NOTARIZE_KEYCHAIN_PROFILE=\"CasparCG-Notarize\""
    echo ""
    echo "Setup keychain profile (one-time):"
    echo "  xcrun notarytool store-credentials \"CasparCG-Notarize\" \\"
    echo "      --apple-id \"your@email.com\" --team-id \"TEAMID\" --password \"xxxx\""
    echo ""
    echo "Examples:"
    echo "  $0                       # Full build: signed, notarized DMG with NDI"
    echo "  $0 --no-sign             # Unsigned app bundle + DMG (no .env needed)"
    echo "  $0 --no-notarize         # Signed but not notarized"
    echo "  $0 --no-dmg --no-sign    # Quick: unsigned app bundle only"
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --no-ndi)
            INCLUDE_NDI=false
            shift
            ;;
        --ndi-path)
            NDI_LIB_PATH="$2"
            shift 2
            ;;
        --no-dmg)
            CREATE_DMG=false
            shift
            ;;
        --no-sign)
            SIGN_APP=false
            NOTARIZE=false
            shift
            ;;
        --no-notarize)
            NOTARIZE=false
            shift
            ;;
        --identity)
            OPT_SIGNING_IDENTITY="$2"
            shift 2
            ;;
        --keychain-profile)
            OPT_KEYCHAIN_PROFILE="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# Auto-disable signing/notarization if credentials are missing
if [ "$SIGN_APP" = true ] && [ -z "$OPT_SIGNING_IDENTITY" ]; then
    echo "Warning: No SIGNING_IDENTITY found in .env or --identity flag. Skipping code signing."
    echo "  To enable, add SIGNING_IDENTITY to .env or pass --identity \"...\""
    SIGN_APP=false
    NOTARIZE=false
fi

if [ "$NOTARIZE" = true ] && [ -z "$OPT_KEYCHAIN_PROFILE" ]; then
    echo "Warning: No NOTARIZE_KEYCHAIN_PROFILE found in .env or --keychain-profile flag. Skipping notarization."
    echo "  To enable, add NOTARIZE_KEYCHAIN_PROFILE to .env or pass --keychain-profile \"...\""
    echo ""
    echo "  Setup keychain profile (one-time):"
    echo "    xcrun notarytool store-credentials \"CasparCG-Notarize\" \\"
    echo "        --apple-id \"your@email.com\" --team-id \"TEAMID\" --password \"xxxx\""
    NOTARIZE=false
fi

# Check build exists
if [ ! -f "$BUILD_DIR/shell/casparcg" ]; then
    echo "Error: Build not found. Build the project using cmake first."
    exit 1
fi

if [ ! -d "$BUILD_DIR/Frameworks/Chromium Embedded Framework.framework" ]; then
    echo "Error: CEF framework not found in build."
    exit 1
fi

echo "========================================"
echo "CasparCG macOS Packaging"
echo "========================================"
echo "Version: $VERSION ($GIT_HASH)"
echo "Include NDI: $INCLUDE_NDI"
echo "Create DMG: $CREATE_DMG"
echo "Sign: $SIGN_APP"
echo "Notarize: $NOTARIZE"
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"

# App bundle paths
APP_BUNDLE="$OUTPUT_DIR/$APP_NAME.app"
CONTENTS="$APP_BUNDLE/Contents"
MACOS="$CONTENTS/MacOS"
FRAMEWORKS="$CONTENTS/Frameworks"
RESOURCES="$CONTENTS/Resources"

# Remove existing bundle
if [ -d "$APP_BUNDLE" ]; then
    echo "Removing existing app bundle..."
    rm -rf "$APP_BUNDLE"
fi

echo "Creating app bundle structure..."
mkdir -p "$MACOS"
mkdir -p "$FRAMEWORKS"
mkdir -p "$RESOURCES"
mkdir -p "$RESOURCES/data"
mkdir -p "$RESOURCES/media"
mkdir -p "$RESOURCES/template"
mkdir -p "$RESOURCES/log"

# Copy main executable
echo "Copying executable..."
cp "$BUILD_DIR/shell/casparcg" "$MACOS/"

# Copy CEF framework
echo "Copying CEF framework..."
cp -R "$BUILD_DIR/Frameworks/Chromium Embedded Framework.framework" "$FRAMEWORKS/"
# Fix read-only file permissions from CEF (causes issues with file transfer tools)
find "$FRAMEWORKS/Chromium Embedded Framework.framework" -type f -perm 444 -exec chmod 644 {} \;

# ============================================================================
# Create CEF Helper app bundles
# ============================================================================
echo "Creating CEF helper app bundles..."

HELPER_PLIST_TEMPLATE="$ROOT_DIR/src/shell/mac/helper-Info.plist.in"

if [ ! -f "$HELPER_PLIST_TEMPLATE" ]; then
    echo "Warning: helper-Info.plist.in not found at $HELPER_PLIST_TEMPLATE, skipping helper creation"
else
    # CEF requires these 5 helper variants (matches cef_variables.cmake)
    # Format: "name_suffix|bundle_id_suffix"
    CEF_HELPERS=(
        "|"
        " (Alerts)|.alerts"
        " (GPU)|.gpu"
        " (Plugin)|.plugin"
        " (Renderer)|.renderer"
    )

    for helper_spec in "${CEF_HELPERS[@]}"; do
        IFS='|' read -r NAME_SUFFIX PLIST_SUFFIX <<< "$helper_spec"

        HELPER_NAME="CasparCG Helper${NAME_SUFFIX}"
        HELPER_APP="$FRAMEWORKS/${HELPER_NAME}.app"
        HELPER_MACOS="$HELPER_APP/Contents/MacOS"

        echo "  Creating: ${HELPER_NAME}.app"
        mkdir -p "$HELPER_MACOS"

        # Copy main binary as the helper executable
        cp "$MACOS/casparcg" "$HELPER_MACOS/${HELPER_NAME}"

        # Generate Info.plist from template
        sed -e "s/\${EXECUTABLE_NAME}/${HELPER_NAME}/g" \
            -e "s/\${PRODUCT_NAME}/${HELPER_NAME}/g" \
            -e "s/\${BUNDLE_ID_SUFFIX}/${PLIST_SUFFIX}/g" \
            "$HELPER_PLIST_TEMPLATE" > "$HELPER_APP/Contents/Info.plist"

        # Fix CEF framework path for helper binary
        # Helpers are at Contents/Frameworks/Helper.app/Contents/MacOS/
        # so they need to go up 4 levels to reach Contents/Frameworks/
        install_name_tool -add_rpath "@executable_path/../../../../Frameworks" \
            "$HELPER_MACOS/${HELPER_NAME}" 2>/dev/null || true
    done
fi

# Create default config file for app bundle with correct paths
echo "Creating app bundle configuration..."
cat > "$RESOURCES/casparcg.config" << 'CONFIGEOF'
<?xml version="1.0" encoding="utf-8"?>
<!--
    CasparCG Server - macOS App Bundle Configuration

    This is the default configuration for the macOS app bundle.

    To customize, copy this file to ~/.config/CasparCG/casparcg.config
    and edit it there. CasparCG will use your custom config if it exists.

    Default paths (relative to the app bundle):
      - Media:     CasparCG.app/Contents/MacOS/media/
      - Templates: CasparCG.app/Contents/MacOS/template/
      - Data:      CasparCG.app/Contents/MacOS/data/
      - Logs:      CasparCG.app/Contents/MacOS/log/
-->
<configuration>
    <paths>
        <media-path>media/</media-path>
        <log-path>log/</log-path>
        <data-path>data/</data-path>
        <template-path>template/</template-path>
    </paths>

    <lock-clear-phrase>secret</lock-clear-phrase>

    <channels>
        <channel>
            <video-mode>720p5000</video-mode>
            <consumers>
                <system-audio />
                <screen>
                    <device>1</device>
                    <windowed>true</windowed>
                </screen>
            </consumers>
        </channel>
    </channels>

    <controllers>
        <tcp>
            <port>5250</port>
            <protocol>AMCP</protocol>
        </tcp>
    </controllers>

    <osc>
        <default-port>6250</default-port>
        <disable-send-to-amcp-clients>true</disable-send-to-amcp-clients>
        <predefined-clients />
    </osc>
</configuration>
CONFIGEOF

# Note: config is copied to ~/Library/Application Support/CasparCG/ at first launch

# Copy data files if they exist (excluding cache directories)
if [ -d "$BUILD_DIR/shell/data" ]; then
    # Use rsync to exclude cache directories
    rsync -a --exclude='cef_cache' --exclude='*.log' "$BUILD_DIR/shell/data/" "$RESOURCES/data/" 2>/dev/null || true
fi

# Copy media files to Resources
# First try build dir, then fall back to tests/testmedia/
if [ -d "$BUILD_DIR/shell/media" ] && [ -n "$(ls -A "$BUILD_DIR/shell/media" 2>/dev/null)" ]; then
    echo "Copying media from build directory..."
    cp -R "$BUILD_DIR/shell/media/"* "$RESOURCES/media/" 2>/dev/null || true
elif [ -d "$ROOT_DIR/tests/testmedia" ]; then
    echo "Copying test media files..."
    cp -R "$ROOT_DIR/tests/testmedia/"* "$RESOURCES/media/" 2>/dev/null || true
fi

# Copy template files to Resources
if [ -d "$BUILD_DIR/shell/template" ] && [ -n "$(ls -A "$BUILD_DIR/shell/template" 2>/dev/null)" ]; then
    cp -R "$BUILD_DIR/shell/template/"* "$RESOURCES/template/" 2>/dev/null || true
fi

# Note: No symlinks created in MacOS directory.
# The launcher script sets up a writable working directory at
# ~/Library/Application Support/CasparCG/ with log/, data/, media/, template/

# Copy app icon
if [ -f "$ROOT_DIR/src/shell/CasparCG.icns" ]; then
    echo "Copying app icon..."
    cp "$ROOT_DIR/src/shell/CasparCG.icns" "$RESOURCES/"
fi

# Copy font for OSD (if it exists) - to Resources only
if [ -f "$BUILD_DIR/shell/LiberationMono-Regular.ttf" ]; then
    cp "$BUILD_DIR/shell/LiberationMono-Regular.ttf" "$RESOURCES/"
fi

# Include NDI if requested
if [ "$INCLUDE_NDI" = true ]; then
    echo "Including NDI library..."

    # Find NDI library
    if [ -n "$NDI_LIB_PATH" ] && [ -f "$NDI_LIB_PATH" ]; then
        NDI_SOURCE="$NDI_LIB_PATH"
    elif [ -f "/usr/local/lib/libndi.dylib" ]; then
        NDI_SOURCE="/usr/local/lib/libndi.dylib"
    elif [ -f "/Library/NDI SDK for Apple/lib/macOS/libndi.dylib" ]; then
        NDI_SOURCE="/Library/NDI SDK for Apple/lib/macOS/libndi.dylib"
    else
        echo "Warning: NDI library not found. Skipping NDI bundling."
        echo "  Install NDI SDK or specify path with --include-ndi /path/to/libndi.dylib"
        INCLUDE_NDI=false
    fi

    if [ "$INCLUDE_NDI" = true ]; then
        cp "$NDI_SOURCE" "$FRAMEWORKS/"
        echo "  Copied: $NDI_SOURCE"
    fi
fi

# ============================================================================
# Bundle all dynamic library dependencies
# ============================================================================
echo ""
echo "Bundling dynamic library dependencies..."

# Track which libraries we've already processed to avoid duplicates
# Using a file instead of associative array for bash 3.x compatibility
PROCESSED_LIBS_FILE=$(mktemp)
trap "rm -f $PROCESSED_LIBS_FILE" EXIT

# Function to check if a library has been processed
is_lib_processed() {
    local lib_name="$1"
    grep -q "^${lib_name}$" "$PROCESSED_LIBS_FILE" 2>/dev/null
}

# Function to mark a library as processed
mark_lib_processed() {
    local lib_name="$1"
    echo "$lib_name" >> "$PROCESSED_LIBS_FILE"
}

# Function to check if a library is a system library (should not be bundled)
is_system_lib() {
    local lib="$1"
    # System libraries that should NOT be bundled
    # Note: @rpath is NOT skipped - we need to resolve and bundle those
    # Note: @loader_path is NOT skipped when processing framework libraries
    if [[ "$lib" == /usr/lib/* ]] || \
       [[ "$lib" == /System/* ]] || \
       [[ "$lib" == @executable_path/* ]]; then
        return 0  # true - is system lib
    fi
    return 1  # false - not system lib
}

# Function to resolve @rpath or @loader_path to actual path
resolve_lib_path() {
    local lib="$1"
    local lib_name=$(basename "$lib")

    # If it's an absolute path that exists, return it
    if [[ "$lib" != @* ]] && [ -f "$lib" ]; then
        echo "$lib"
        return 0
    fi

    # If it's @rpath or @loader_path, search for the library
    if [[ "$lib" == @rpath/* ]] || [[ "$lib" == @loader_path/* ]]; then
        # Search in common Homebrew locations
        local search_paths=(
            "/opt/homebrew/lib"
            "/opt/homebrew/opt/*/lib"
            "/usr/local/lib"
            "/usr/local/opt/*/lib"
        )

        for search_path in "${search_paths[@]}"; do
            # Use glob expansion
            for dir in $search_path; do
                if [ -f "$dir/$lib_name" ]; then
                    echo "$dir/$lib_name"
                    return 0
                fi
            done
        done
    fi

    # If it's a regular path, try to find it
    if [ -f "$lib" ]; then
        echo "$lib"
        return 0
    fi

    # Not found
    return 1
}

# Function to get the real path of a library (resolving symlinks)
get_real_lib_path() {
    local lib="$1"
    if [ -L "$lib" ]; then
        # Resolve symlink - use python3 as readlink -f doesn't work on macOS
        python3 -c "import os; print(os.path.realpath('$lib'))" 2>/dev/null || echo "$lib"
    else
        echo "$lib"
    fi
}

# Function to copy a library (without fixing paths - that happens later)
copy_lib() {
    local lib_path="$1"
    local lib_name=$(basename "$lib_path")

    # Skip if already processed
    if is_lib_processed "$lib_name"; then
        return 0
    fi

    # Skip system libraries
    if is_system_lib "$lib_path"; then
        return 0
    fi

    # Resolve @rpath or @loader_path to actual path
    if [[ "$lib_path" == @rpath/* ]] || [[ "$lib_path" == @loader_path/* ]]; then
        local resolved=$(resolve_lib_path "$lib_path")
        if [ -n "$resolved" ]; then
            lib_path="$resolved"
        else
            echo "  Warning: Could not resolve: $lib_path"
            return 0
        fi
    fi

    # Skip if file doesn't exist
    if [ ! -f "$lib_path" ]; then
        echo "  Warning: Library not found: $lib_path"
        return 0
    fi

    # Get real path (resolve symlinks)
    local real_path=$(get_real_lib_path "$lib_path")
    local real_name=$(basename "$real_path")

    # Mark as processed (both original name and real name)
    mark_lib_processed "$lib_name"
    mark_lib_processed "$real_name"

    # Copy the library if not already in Frameworks
    if [ ! -f "$FRAMEWORKS/$real_name" ]; then
        echo "  Copying: $real_path"
        cp "$real_path" "$FRAMEWORKS/"
        chmod 755 "$FRAMEWORKS/$real_name"

        # Also create symlink if original name differs
        if [ "$lib_name" != "$real_name" ] && [ ! -e "$FRAMEWORKS/$lib_name" ]; then
            ln -s "$real_name" "$FRAMEWORKS/$lib_name"
        fi
    fi

    # Fix the install name of the copied library itself
    install_name_tool -id "@executable_path/../Frameworks/$real_name" "$FRAMEWORKS/$real_name" 2>/dev/null || true

    # Recursively discover and copy dependencies of this library
    discover_dependencies "$FRAMEWORKS/$real_name"
}

# Function to discover all dependencies (copy phase only, no path fixing)
discover_dependencies() {
    local binary="$1"

    # Get all linked libraries
    local deps=$(otool -L "$binary" 2>/dev/null | tail -n +2 | awk '{print $1}')

    for dep in $deps; do
        # Skip system libraries
        if is_system_lib "$dep"; then
            continue
        fi

        # Copy the dependency if needed (recursive)
        copy_lib "$dep"
    done
}

# Phase 1: Discover and copy all dependencies (no path fixing yet)
echo "Discovering and copying dependencies..."
discover_dependencies "$MACOS/casparcg"

# Also discover dependencies from any pre-existing libraries (CEF, NDI)
for lib in "$FRAMEWORKS"/*.dylib; do
    if [ -f "$lib" ] && [ ! -L "$lib" ]; then
        discover_dependencies "$lib"
    fi
done

# ============================================================================
# Handle Vulkan ICD (Installable Client Driver) configuration
# ============================================================================
echo ""
echo "Configuring Vulkan ICD..."

# Find MoltenVK library - it's needed for Vulkan on macOS
MOLTENVK_LIB=""
MOLTENVK_ICD=""

# Check common locations for MoltenVK
# Homebrew on Apple Silicon
if [ -f "/opt/homebrew/opt/molten-vk/lib/libMoltenVK.dylib" ]; then
    MOLTENVK_LIB="/opt/homebrew/opt/molten-vk/lib/libMoltenVK.dylib"
    # ICD is in etc/ not share/ for Homebrew
    MOLTENVK_ICD="/opt/homebrew/opt/molten-vk/etc/vulkan/icd.d/MoltenVK_icd.json"
# Homebrew on Intel
elif [ -f "/usr/local/opt/molten-vk/lib/libMoltenVK.dylib" ]; then
    MOLTENVK_LIB="/usr/local/opt/molten-vk/lib/libMoltenVK.dylib"
    MOLTENVK_ICD="/usr/local/opt/molten-vk/etc/vulkan/icd.d/MoltenVK_icd.json"
# LunarG Vulkan SDK
elif [ -f "$HOME/VulkanSDK/latest/macOS/lib/libMoltenVK.dylib" ]; then
    MOLTENVK_LIB="$HOME/VulkanSDK/latest/macOS/lib/libMoltenVK.dylib"
    MOLTENVK_ICD="$HOME/VulkanSDK/latest/macOS/share/vulkan/icd.d/MoltenVK_icd.json"
# Try to find anywhere
else
    # Search for library
    MOLTENVK_LIB=$(find /opt/homebrew /usr/local -name "libMoltenVK.dylib" 2>/dev/null | head -1)
    # Search for ICD JSON
    MOLTENVK_ICD=$(find /opt/homebrew /usr/local -name "MoltenVK_icd.json" 2>/dev/null | head -1)
fi

if [ -n "$MOLTENVK_LIB" ] && [ -f "$MOLTENVK_LIB" ]; then
    echo "  Found MoltenVK: $MOLTENVK_LIB"

    # Copy MoltenVK library
    if [ ! -f "$FRAMEWORKS/libMoltenVK.dylib" ]; then
        cp "$MOLTENVK_LIB" "$FRAMEWORKS/"
        chmod 755 "$FRAMEWORKS/libMoltenVK.dylib"
        install_name_tool -id "@executable_path/../Frameworks/libMoltenVK.dylib" "$FRAMEWORKS/libMoltenVK.dylib" 2>/dev/null || true
    fi

    # Create vulkan ICD directory structure
    mkdir -p "$RESOURCES/vulkan/icd.d"

    # Create ICD JSON that points to our bundled MoltenVK
    cat > "$RESOURCES/vulkan/icd.d/MoltenVK_icd.json" << 'ICDJSON'
{
    "file_format_version": "1.0.0",
    "ICD": {
        "library_path": "../../../Frameworks/libMoltenVK.dylib",
        "api_version": "1.2.0"
    }
}
ICDJSON
    echo "  Created Vulkan ICD configuration"

    # Discover MoltenVK dependencies
    discover_dependencies "$FRAMEWORKS/libMoltenVK.dylib"
else
    echo "  Warning: MoltenVK not found. Vulkan may not work without it."
    echo "  Install via: brew install molten-vk"
fi

# ============================================================================
# Create wrapper script to set up Vulkan environment
# ============================================================================
echo "Creating launcher script..."

cat > "$MACOS/casparcg-launcher" << 'LAUNCHER'
#!/bin/bash
# CasparCG Launcher - Sets up environment for bundled libraries
# Run this directly from terminal for headless/manual operation

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONTENTS_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
FRAMEWORKS_DIR="$CONTENTS_DIR/Frameworks"
RESOURCES_DIR="$CONTENTS_DIR/Resources"

# Set up writable working directory outside the signed app bundle
WORK_DIR="$HOME/Library/Application Support/CasparCG"
mkdir -p "$WORK_DIR/log"
mkdir -p "$WORK_DIR/data"
mkdir -p "$WORK_DIR/media"
mkdir -p "$WORK_DIR/template"

# Copy default config if user doesn't have one yet
if [ ! -f "$WORK_DIR/casparcg.config" ]; then
    cp "$RESOURCES_DIR/casparcg.config" "$WORK_DIR/casparcg.config"
    echo "Created default config at: $WORK_DIR/casparcg.config"
fi

# Copy bundled media to working dir if media dir is empty
if [ -z "$(ls -A "$WORK_DIR/media" 2>/dev/null)" ] && [ -d "$RESOURCES_DIR/media" ]; then
    cp -R "$RESOURCES_DIR/media/"* "$WORK_DIR/media/" 2>/dev/null || true
fi

# Copy bundled templates to working dir if template dir is empty
if [ -z "$(ls -A "$WORK_DIR/template" 2>/dev/null)" ] && [ -d "$RESOURCES_DIR/template" ]; then
    cp -R "$RESOURCES_DIR/template/"* "$WORK_DIR/template/" 2>/dev/null || true
fi

# Always update shaders from bundle (they should match the binary)
if [ -d "$RESOURCES_DIR/shaders" ]; then
    mkdir -p "$WORK_DIR/shaders"
    cp -R "$RESOURCES_DIR/shaders/"* "$WORK_DIR/shaders/" 2>/dev/null || true
fi

# Change to writable working directory
cd "$WORK_DIR"

# Set library paths for bundled dylibs
export DYLD_LIBRARY_PATH="$FRAMEWORKS_DIR:$DYLD_LIBRARY_PATH"
export DYLD_FRAMEWORK_PATH="$FRAMEWORKS_DIR:$DYLD_FRAMEWORK_PATH"

# Set Vulkan ICD path to use bundled MoltenVK
export VK_ICD_FILENAMES="$RESOURCES_DIR/vulkan/icd.d/MoltenVK_icd.json"
export VK_DRIVER_FILES="$RESOURCES_DIR/vulkan/icd.d/MoltenVK_icd.json"

# Set NDI runtime directory
export NDI_RUNTIME_DIR_V6="$FRAMEWORKS_DIR"

# Launch CasparCG
exec "$SCRIPT_DIR/casparcg" "$@"
LAUNCHER

chmod +x "$MACOS/casparcg-launcher"

# ============================================================================
# Create Terminal-opening launcher for double-click from Finder
# ============================================================================
echo "Creating Terminal launcher for Finder..."

cat > "$MACOS/CasparCG-Terminal" << 'TERMLAUNCHER'
#!/bin/bash
# CasparCG Terminal Launcher
# This script opens Terminal.app and runs CasparCG inside it
# Used when double-clicking the .app bundle from Finder

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LAUNCHER_PATH="$SCRIPT_DIR/casparcg-launcher"

# Use osascript to open Terminal and run the launcher
osascript <<EOF
tell application "Terminal"
    activate
    do script "\"$LAUNCHER_PATH\"; exit"
end tell
EOF
TERMLAUNCHER

chmod +x "$MACOS/CasparCG-Terminal"

# ============================================================================
# Final pass: Fix all remaining library paths in all binaries
# ============================================================================
echo ""
echo "Final pass: fixing all library paths..."

# Function to fix all Homebrew paths in a binary
fix_all_lib_paths() {
    local binary="$1"
    local deps=$(otool -L "$binary" 2>/dev/null | tail -n +2 | awk '{print $1}')

    for dep in $deps; do
        # Skip if already using @executable_path or system path
        if [[ "$dep" == @executable_path/* ]] || \
           [[ "$dep" == /usr/lib/* ]] || \
           [[ "$dep" == /System/* ]]; then
            continue
        fi

        local dep_name=$(basename "$dep")

        # Check if we have this library in Frameworks
        if [ -f "$FRAMEWORKS/$dep_name" ] || [ -L "$FRAMEWORKS/$dep_name" ]; then
            echo "  Fixing: $dep_name in $(basename "$binary")"
            install_name_tool -change "$dep" "@executable_path/../Frameworks/$dep_name" "$binary" 2>/dev/null || true
        fi
    done
}

# Fix paths in main executable
fix_all_lib_paths "$MACOS/casparcg"

# Fix paths in all framework libraries
for lib in "$FRAMEWORKS"/*.dylib; do
    if [ -f "$lib" ] && [ ! -L "$lib" ]; then
        fix_all_lib_paths "$lib"
    fi
done

# Fix paths in CEF helper app executables
for helper_app in "$FRAMEWORKS/CasparCG Helper"*.app; do
    if [ -d "$helper_app" ]; then
        for helper_exe in "$helper_app/Contents/MacOS/"*; do
            if [ -f "$helper_exe" ]; then
                fix_all_lib_paths "$helper_exe"
                # Fix CEF framework path for helper binary
                install_name_tool -change \
                    "@rpath/Chromium Embedded Framework.framework/Chromium Embedded Framework" \
                    "@executable_path/../../../../Frameworks/Chromium Embedded Framework.framework/Chromium Embedded Framework" \
                    "$helper_exe" 2>/dev/null || true
                install_name_tool -change \
                    "/Library/Frameworks/Chromium Embedded Framework.framework/Chromium Embedded Framework" \
                    "@executable_path/../../../../Frameworks/Chromium Embedded Framework.framework/Chromium Embedded Framework" \
                    "$helper_exe" 2>/dev/null || true
            fi
        done
    fi
done

echo ""
echo "Bundled libraries:"
ls -la "$FRAMEWORKS"/*.dylib 2>/dev/null | awk '{print "  " $NF}' | xargs -I {} basename {} || true
echo ""

# Create Info.plist
# Note: We use casparcg-launcher as the executable to set up library paths
echo "Creating Info.plist..."
cat > "$CONTENTS/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleExecutable</key>
    <string>CasparCG-Terminal</string>
    <key>CFBundleIdentifier</key>
    <string>$BUNDLE_ID</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleIconFile</key>
    <string>CasparCG</string>
    <key>CFBundleName</key>
    <string>$APP_NAME</string>
    <key>CFBundleDisplayName</key>
    <string>CasparCG Server</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>$VERSION</string>
    <key>CFBundleVersion</key>
    <string>$VERSION.$GIT_HASH</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.15</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSSupportsAutomaticGraphicsSwitching</key>
    <true/>
    <key>NSPrincipalClass</key>
    <string>NSApplication</string>
    <key>LSApplicationCategoryType</key>
    <string>public.app-category.video</string>
    <key>NSHumanReadableCopyright</key>
    <string>Copyright CasparCG Project. Licensed under GPLv3.</string>
</dict>
</plist>
EOF

# Create entitlements file for signing
ENTITLEMENTS_FILE="$OUTPUT_DIR/CasparCG.entitlements"
cat > "$ENTITLEMENTS_FILE" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>com.apple.security.cs.allow-jit</key>
    <true/>
    <key>com.apple.security.cs.allow-unsigned-executable-memory</key>
    <true/>
    <key>com.apple.security.cs.disable-library-validation</key>
    <true/>
    <key>com.apple.security.network.server</key>
    <true/>
    <key>com.apple.security.network.client</key>
    <true/>
    <key>com.apple.security.device.camera</key>
    <true/>
    <key>com.apple.security.device.audio-input</key>
    <true/>
</dict>
</plist>
EOF

# Fix library paths with install_name_tool
echo "Fixing library paths..."

# Fix CEF framework install name in main executable
install_name_tool -change \
    "@rpath/Chromium Embedded Framework.framework/Chromium Embedded Framework" \
    "@executable_path/../Frameworks/Chromium Embedded Framework.framework/Chromium Embedded Framework" \
    "$MACOS/casparcg" 2>/dev/null || true

# Also try the absolute path pattern
install_name_tool -change \
    "/Library/Frameworks/Chromium Embedded Framework.framework/Chromium Embedded Framework" \
    "@executable_path/../Frameworks/Chromium Embedded Framework.framework/Chromium Embedded Framework" \
    "$MACOS/casparcg" 2>/dev/null || true

# Fix NDI if bundled
if [ "$INCLUDE_NDI" = true ] && [ -f "$FRAMEWORKS/libndi.dylib" ]; then
    # Update the install name of the NDI library itself
    install_name_tool -id "@executable_path/../Frameworks/libndi.dylib" "$FRAMEWORKS/libndi.dylib" 2>/dev/null || true
fi

# Ad-hoc sign bundled libraries if not doing formal signing
# This is required on macOS because copied dylibs lose their valid signature
if [ "$SIGN_APP" = false ]; then
    echo "Ad-hoc signing bundled libraries..."

    # Ad-hoc sign all dylibs in Frameworks
    for lib in "$FRAMEWORKS"/*.dylib; do
        if [ -f "$lib" ]; then
            echo "  Ad-hoc signing: $(basename "$lib")"
            codesign --force --sign - "$lib" 2>/dev/null || true
        fi
    done

    # Ad-hoc sign CEF framework (required for dlopen)
    if [ -d "$FRAMEWORKS/Chromium Embedded Framework.framework" ]; then
        echo "  Ad-hoc signing CEF framework..."
        codesign --force --deep --sign - "$FRAMEWORKS/Chromium Embedded Framework.framework" 2>/dev/null || true
    fi

    # Ad-hoc sign CEF helper apps
    for helper_app in "$FRAMEWORKS/CasparCG Helper"*.app; do
        if [ -d "$helper_app" ]; then
            echo "  Ad-hoc signing: $(basename "$helper_app")"
            codesign --force --deep --sign - "$helper_app" 2>/dev/null || true
        fi
    done

    # Ad-hoc sign the main executable
    echo "  Ad-hoc signing main executable..."
    codesign --force --sign - "$MACOS/casparcg" 2>/dev/null || true

    # Ad-hoc sign the launcher scripts (if they exist)
    if [ -f "$MACOS/casparcg-launcher" ]; then
        echo "  Ad-hoc signing casparcg-launcher..."
        codesign --force --sign - "$MACOS/casparcg-launcher" 2>/dev/null || true
    fi
    if [ -f "$MACOS/CasparCG-Terminal" ]; then
        echo "  Ad-hoc signing CasparCG-Terminal..."
        codesign --force --sign - "$MACOS/CasparCG-Terminal" 2>/dev/null || true
    fi
fi

# Code signing
if [ "$SIGN_APP" = true ]; then
    echo ""
    echo "Signing app bundle..."
    echo "Identity: $OPT_SIGNING_IDENTITY"

    # Sign all dylibs in Frameworks first (inside-out signing)
    echo "  Signing bundled libraries..."
    for lib in "$FRAMEWORKS"/*.dylib; do
        if [ -f "$lib" ]; then
            echo "    Signing: $(basename "$lib")"
            codesign --force --options runtime --timestamp \
                --sign "$OPT_SIGNING_IDENTITY" \
                "$lib"
        fi
    done

    # Sign CEF framework (must sign nested libraries first)
    if [ -d "$FRAMEWORKS/Chromium Embedded Framework.framework" ]; then
        echo "  Signing CEF framework libraries..."
        # Sign all dylibs in Libraries subdirectory first
        if [ -d "$FRAMEWORKS/Chromium Embedded Framework.framework/Libraries" ]; then
            for lib in "$FRAMEWORKS/Chromium Embedded Framework.framework/Libraries"/*.dylib; do
                if [ -f "$lib" ]; then
                    echo "    Signing: $(basename "$lib")"
                    codesign --force --options runtime --timestamp \
                        --sign "$OPT_SIGNING_IDENTITY" \
                        "$lib"
                fi
            done
        fi
        # Sign any helper apps
        for helper in "$FRAMEWORKS/Chromium Embedded Framework.framework/Helpers"/*.app; do
            if [ -d "$helper" ]; then
                echo "    Signing: $(basename "$helper")"
                codesign --force --options runtime --timestamp \
                    --entitlements "$ENTITLEMENTS_FILE" \
                    --sign "$OPT_SIGNING_IDENTITY" \
                    "$helper"
            fi
        done
        # Now sign the framework itself
        echo "  Signing CEF framework..."
        codesign --force --options runtime --timestamp \
            --entitlements "$ENTITLEMENTS_FILE" \
            --sign "$OPT_SIGNING_IDENTITY" \
            "$FRAMEWORKS/Chromium Embedded Framework.framework"
    fi

    # Sign CEF helper apps (must be signed before the main bundle)
    for helper_app in "$FRAMEWORKS/CasparCG Helper"*.app; do
        if [ -d "$helper_app" ]; then
            echo "  Signing: $(basename "$helper_app")"
            codesign --force --options runtime --timestamp \
                --entitlements "$ENTITLEMENTS_FILE" \
                --sign "$OPT_SIGNING_IDENTITY" \
                "$helper_app"
        fi
    done

    # Sign main executable
    echo "  Signing main executable..."
    codesign --force --options runtime --timestamp \
        --entitlements "$ENTITLEMENTS_FILE" \
        --sign "$OPT_SIGNING_IDENTITY" \
        "$MACOS/casparcg"

    # Sign launcher scripts if they exist
    if [ -f "$MACOS/casparcg-launcher" ]; then
        echo "  Signing casparcg-launcher..."
        codesign --force --options runtime --timestamp \
            --sign "$OPT_SIGNING_IDENTITY" \
            "$MACOS/casparcg-launcher"
    fi
    if [ -f "$MACOS/CasparCG-Terminal" ]; then
        echo "  Signing CasparCG-Terminal..."
        codesign --force --options runtime --timestamp \
            --sign "$OPT_SIGNING_IDENTITY" \
            "$MACOS/CasparCG-Terminal"
    fi

    # Sign the bundle
    echo "  Signing app bundle..."
    codesign --force --options runtime --timestamp \
        --entitlements "$ENTITLEMENTS_FILE" \
        --sign "$OPT_SIGNING_IDENTITY" \
        "$APP_BUNDLE"

    # Verify signature
    echo ""
    echo "Verifying signatures..."
    echo "  Main executable:"
    codesign --verify --strict --verbose=2 "$MACOS/casparcg"
    echo "  App bundle:"
    codesign --verify --strict --verbose=2 "$APP_BUNDLE"

    echo ""
    echo "Checking Gatekeeper assessment..."
    spctl --assess --type execute --verbose=2 "$APP_BUNDLE" || echo "Note: Gatekeeper may require notarization for full approval"
fi

# Create DMG
DMG_PATH=""
if [ "$CREATE_DMG" = true ]; then
    echo ""
    echo "Creating DMG..."
    DMG_NAME="CasparCG-${VERSION}-macOS"
    DMG_PATH="$OUTPUT_DIR/$DMG_NAME.dmg"

    # Remove existing DMG
    rm -f "$DMG_PATH"

    # Create staging directory with app and Applications symlink
    DMG_STAGING="$OUTPUT_DIR/dmg-staging"
    rm -rf "$DMG_STAGING"
    mkdir -p "$DMG_STAGING"
    cp -R "$APP_BUNDLE" "$DMG_STAGING/"
    ln -s /Applications "$DMG_STAGING/Applications"

    # Create DMG
    # Note: volume name must differ from app name to avoid macOS Gatekeeper conflict
    hdiutil create -volname "$APP_NAME Server" \
        -srcfolder "$DMG_STAGING" \
        -ov -format UDZO \
        "$DMG_PATH"

    # Clean up staging
    rm -rf "$DMG_STAGING"

    # Sign DMG if signing is enabled
    if [ "$SIGN_APP" = true ]; then
        echo "Signing DMG..."
        codesign --force --timestamp --sign "$OPT_SIGNING_IDENTITY" "$DMG_PATH"
    fi

    echo "DMG created: $DMG_PATH"
fi

# Notarization
if [ "$NOTARIZE" = true ]; then
    echo ""
    echo "Submitting for notarization..."

    if [ -z "$DMG_PATH" ]; then
        # Create a ZIP for notarization if no DMG
        ZIP_PATH="$OUTPUT_DIR/CasparCG-${VERSION}-macOS.zip"
        ditto -c -k --keepParent "$APP_BUNDLE" "$ZIP_PATH"
        NOTARIZE_PATH="$ZIP_PATH"
    else
        NOTARIZE_PATH="$DMG_PATH"
    fi

    echo "Submitting: $NOTARIZE_PATH"

    # Submit for notarization and wait
    xcrun notarytool submit "$NOTARIZE_PATH" \
        --keychain-profile "$OPT_KEYCHAIN_PROFILE" \
        --wait

    # Staple the ticket
    echo ""
    echo "Stapling notarization ticket..."
    if [ -n "$DMG_PATH" ]; then
        xcrun stapler staple "$DMG_PATH"
    else
        xcrun stapler staple "$APP_BUNDLE"
    fi

    echo ""
    echo "Verifying notarization..."
    if [ -n "$DMG_PATH" ]; then
        spctl --assess --type install --verbose=4 "$DMG_PATH"
    else
        spctl --assess --type execute --verbose=4 "$APP_BUNDLE"
    fi
fi

# Clean up
rm -f "$ENTITLEMENTS_FILE"

echo ""
echo "========================================"
echo "Packaging complete!"
echo "========================================"
echo ""
echo "App bundle: $APP_BUNDLE"
if [ -n "$DMG_PATH" ]; then
    echo "DMG: $DMG_PATH"
fi
echo ""
echo "To run the app (opens in Terminal.app):"
echo "  open $APP_BUNDLE"
echo ""
echo "Or run directly from terminal:"
echo "  $APP_BUNDLE/Contents/MacOS/casparcg-launcher"
echo ""
echo "Working directory: ~/Library/Application Support/CasparCG/"
echo "  Config:    ~/Library/Application Support/CasparCG/casparcg.config"
echo "  Media:     ~/Library/Application Support/CasparCG/media/"
echo "  Templates: ~/Library/Application Support/CasparCG/template/"
echo "  Logs:      ~/Library/Application Support/CasparCG/log/"
