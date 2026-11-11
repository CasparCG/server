#!/bin/sh
# Assemble CasparCG.app:
#   Contents/MacOS/casparcg
#   Contents/Frameworks/Chromium Embedded Framework.framework   (if HTML/CEF is enabled)
#   Contents/Frameworks/casparcg Helper{, (GPU), (Renderer), (Plugin), (Alerts)}.app
#
# usage: make_bundle.sh <casparcg executable> <output .app> [<CEF framework dir>] [<version>]
set -e

EXE="$1"
APP="$2"
FRAMEWORK="$3"
VERSION="${4:-2.6.0}"
BUNDLE_ID="com.casparcg.server"

write_plist() { # <path> <executable> <bundle id> <name> <is helper>
    cat > "$1" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key><string>en</string>
    <key>CFBundleExecutable</key><string>$2</string>
    <key>CFBundleIdentifier</key><string>$3</string>
    <key>CFBundleInfoDictionaryVersion</key><string>6.0</string>
    <key>CFBundleName</key><string>$4</string>
    <key>CFBundlePackageType</key><string>APPL</string>
    <key>CFBundleShortVersionString</key><string>$VERSION</string>
    <key>CFBundleVersion</key><string>$VERSION</string>
    <key>LSMinimumSystemVersion</key><string>13.0</string>
$( [ "$5" = 1 ] && echo '    <key>LSUIElement</key><string>1</string>' || echo '    <key>CFBundleIconFile</key><string>AppIcon</string>' )
    <key>NSHighResolutionCapable</key><true/>
    <key>NSSupportsAutomaticGraphicsSwitching</key><true/>
$6
</dict>
</plist>
PLIST
}

rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Frameworks"
cp "$EXE" "$APP/Contents/MacOS/casparcg"
write_plist "$APP/Contents/Info.plist" casparcg "$BUNDLE_ID" CasparCG

SHELL_DIR="$(dirname "$0")/../../src/shell"
mkdir -p "$APP/Contents/Resources"

# Default configuration, copied to ~/Library/Application Support/CasparCG when the app is
# started from Finder without a casparcg.config (see macos_main.mm)
cp "$SHELL_DIR/casparcg.config" "$APP/Contents/Resources/casparcg.config"

# App icon (from the Windows .ico)
ICONSET="$(mktemp -d)/AppIcon.iconset"
mkdir -p "$ICONSET"
if sips -s format png "$SHELL_DIR/CasparCG.ico" --out "$ICONSET/base.png" >/dev/null 2>&1; then
    for S in 16 32 128 256; do
        sips -z $S $S "$ICONSET/base.png" --out "$ICONSET/icon_${S}x${S}.png" >/dev/null
        S2=$((S * 2))
        sips -z $S2 $S2 "$ICONSET/base.png" --out "$ICONSET/icon_${S}x${S}@2x.png" >/dev/null
    done
    rm "$ICONSET/base.png"
    iconutil -c icns "$ICONSET" -o "$APP/Contents/Resources/AppIcon.icns" || true
fi
rm -rf "$(dirname "$ICONSET")"

# Font used by the diagnostics window (DIAG)
FONT="$(dirname "$0")/../../src/shell/liberation-fonts/LiberationMono-Regular.ttf"
if [ -f "$FONT" ]; then
    mkdir -p "$APP/Contents/Resources"
    cp "$FONT" "$APP/Contents/Resources/"
fi

# Optional: bundle the NDI runtime (set NDI_LIB=/path/to/libndi.dylib, e.g. from the NDI SDK for Apple)
if [ -n "$NDI_LIB" ] && [ -f "$NDI_LIB" ]; then
    cp "$NDI_LIB" "$APP/Contents/Frameworks/libndi.dylib"
fi

# Make the app self-contained: copy Homebrew dylibs into Contents/Frameworks (@rpath).
# The Vulkan loader and MoltenVK are loaded with dlopen() at runtime, so they're listed explicitly.
BREW_PREFIX=$(brew --prefix 2>/dev/null || echo /opt/homebrew)
python3 "$(dirname "$0")/bundle_dylibs.py" "$APP" \
    "$BREW_PREFIX/lib/libvulkan.1.dylib" \
    "$BREW_PREFIX/lib/libMoltenVK.dylib"

# Vulkan driver manifest for the bundled MoltenVK (the loader also searches Contents/Resources/vulkan/icd.d)
mkdir -p "$APP/Contents/Resources/vulkan/icd.d"
cat > "$APP/Contents/Resources/vulkan/icd.d/MoltenVK_icd.json" <<ICD
{
    "file_format_version" : "1.0.0",
    "ICD": {
        "library_path": "../../../Frameworks/libMoltenVK.dylib",
        "api_version" : "1.4.0",
        "is_portability_driver" : true
    }
}
ICD

if [ -n "$FRAMEWORK" ] && [ -d "$FRAMEWORK" ]; then
    # ditto preserves the framework layout and code signature
    ditto "$FRAMEWORK" "$APP/Contents/Frameworks/Chromium Embedded Framework.framework"

    for SUFFIX in "" " (GPU)" " (Renderer)" " (Plugin)" " (Alerts)"; do
        NAME="casparcg Helper$SUFFIX"
        ID_SUFFIX=$(echo "$SUFFIX" | tr -d ' ()' | tr 'A-Z' 'a-z')
        HELPER="$APP/Contents/Frameworks/$NAME.app"
        mkdir -p "$HELPER/Contents/MacOS"
        # Hard link: one copy of the binary on disk
        ln "$APP/Contents/MacOS/casparcg" "$HELPER/Contents/MacOS/$NAME" 2>/dev/null ||
            cp "$APP/Contents/MacOS/casparcg" "$HELPER/Contents/MacOS/$NAME"
        # Same as CEF's helper Info.plist template
        write_plist "$HELPER/Contents/Info.plist" "$NAME" "$BUNDLE_ID.helper${ID_SUFFIX:+.$ID_SUFFIX}" "$NAME" 1 \
            "    <key>LSEnvironment</key><dict><key>MallocNanoZone</key><string>0</string></dict>
    <key>LSFileQuarantineEnabled</key><true/>"
    done
fi

# Apple Silicon requires valid signatures; ad-hoc sign the bundle structure.
codesign --force --sign - --timestamp=none "$APP/Contents/MacOS/casparcg" >/dev/null 2>&1 || true
for H in "$APP"/Contents/Frameworks/*.app; do
    [ -d "$H" ] && codesign --force --sign - --timestamp=none "$H" >/dev/null 2>&1 || true
done
codesign --force --sign - --timestamp=none "$APP" >/dev/null 2>&1 || true

echo "Created $APP"
