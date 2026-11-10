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
    <key>LSUIElement</key><string>1</string>
    <key>NSHighResolutionCapable</key><true/>
    <key>NSSupportsAutomaticGraphicsSwitching</key><true/>
</dict>
</plist>
PLIST
}

rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Frameworks"
cp "$EXE" "$APP/Contents/MacOS/casparcg"
write_plist "$APP/Contents/Info.plist" casparcg "$BUNDLE_ID" CasparCG

# Font used by the diagnostics window (DIAG)
FONT="$(dirname "$0")/../../src/shell/liberation-fonts/LiberationMono-Regular.ttf"
if [ -f "$FONT" ]; then
    mkdir -p "$APP/Contents/Resources"
    cp "$FONT" "$APP/Contents/Resources/"
fi

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
        write_plist "$HELPER/Contents/Info.plist" "$NAME" "$BUNDLE_ID.helper${ID_SUFFIX:+.$ID_SUFFIX}" "$NAME"
    done
fi

# Apple Silicon requires valid signatures; ad-hoc sign the bundle structure.
codesign --force --sign - --timestamp=none "$APP/Contents/MacOS/casparcg" >/dev/null 2>&1 || true
for H in "$APP"/Contents/Frameworks/*.app; do
    [ -d "$H" ] && codesign --force --sign - --timestamp=none "$H" >/dev/null 2>&1 || true
done
codesign --force --sign - --timestamp=none "$APP" >/dev/null 2>&1 || true

echo "Created $APP"
