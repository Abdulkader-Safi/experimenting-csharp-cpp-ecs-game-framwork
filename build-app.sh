#!/bin/bash
set -e

BUILD_DIR="build"
APP_NAME="Viewer"
VK_ICD="/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json"

APP_BUNDLE="$BUILD_DIR/$APP_NAME.app"
APP_CONTENTS="$APP_BUNDLE/Contents"
APP_MACOS="$APP_CONTENTS/MacOS"
APP_RESOURCES="$APP_CONTENTS/Resources"

rm -rf "$APP_BUNDLE"
mkdir -p "$APP_MACOS" "$APP_RESOURCES/build/shaders"

cp "$BUILD_DIR/Viewer.exe" "$APP_RESOURCES/"
cp "$BUILD_DIR/librenderer.dylib" "$APP_RESOURCES/"
cp "$BUILD_DIR/shaders/vert.spv" "$BUILD_DIR/shaders/frag.spv" "$APP_RESOURCES/build/shaders/"
if [ -d models ]; then cp -r models "$APP_RESOURCES/"; fi

# Launcher script
cat > "$APP_MACOS/$APP_NAME" <<'LAUNCHER'
#!/bin/bash
DIR="$(cd "$(dirname "$0")/../Resources" && pwd)"
export DYLD_LIBRARY_PATH="$DIR:/opt/homebrew/lib"
export VK_ICD_FILENAMES=/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json
cd "$DIR"
exec /opt/homebrew/bin/mono "$DIR/Viewer.exe" "$@"
LAUNCHER
chmod +x "$APP_MACOS/$APP_NAME"

# Info.plist
cat > "$APP_CONTENTS/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleName</key>
  <string>$APP_NAME</string>
  <key>CFBundleExecutable</key>
  <string>$APP_NAME</string>
  <key>CFBundleIdentifier</key>
  <string>com.viewer.app</string>
  <key>CFBundleVersion</key>
  <string>1.0</string>
  <key>CFBundlePackageType</key>
  <string>APPL</string>
</dict>
</plist>
PLIST

echo "Built $APP_BUNDLE"
