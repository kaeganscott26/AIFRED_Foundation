#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SDK_ROOT="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-$HOME/Android/Sdk}}"
APP_DIR="$ROOT_DIR/android/app/src/main"
BUILD_DIR="$ROOT_DIR/android/build"
DIST_DIR="$ROOT_DIR/dist/android"
DEBUG_KEYSTORE="${DEBUG_KEYSTORE:-$HOME/.android/debug.keystore}"
KEY_ALIAS="androiddebugkey"
KEY_PASS="android"
STORE_PASS="android"

if [ ! -d "$SDK_ROOT" ]; then
  echo "Android SDK not found at $SDK_ROOT" >&2
  exit 1
fi

BUILD_TOOLS_VERSION="$(ls -1 "$SDK_ROOT/build-tools" | sort -V | tail -n1)"
PLATFORM_NAME="$(ls -1 "$SDK_ROOT/platforms" | rg '^android-[0-9]+$' | sort -V | tail -n1)"

if [ -z "$BUILD_TOOLS_VERSION" ] || [ -z "$PLATFORM_NAME" ]; then
  echo "Missing Android build-tools or platforms in $SDK_ROOT" >&2
  exit 1
fi

TARGET_SDK="${PLATFORM_NAME#android-}"
BUILD_TOOLS_DIR="$SDK_ROOT/build-tools/$BUILD_TOOLS_VERSION"
ANDROID_JAR="$SDK_ROOT/platforms/$PLATFORM_NAME/android.jar"

AAPT2="$BUILD_TOOLS_DIR/aapt2"
D8="$BUILD_TOOLS_DIR/d8"
ZIPALIGN="$BUILD_TOOLS_DIR/zipalign"
APKSIGNER="$BUILD_TOOLS_DIR/apksigner"

for tool in "$AAPT2" "$D8" "$ZIPALIGN" "$APKSIGNER" "$ANDROID_JAR"; do
  if [ ! -e "$tool" ]; then
    echo "Required Android tool not found: $tool" >&2
    exit 1
  fi
done

"$ROOT_DIR/scripts/sync_android_assets.sh"

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"/{compiled_res,gen,classes,dex}
mkdir -p "$DIST_DIR"

MANIFEST="$APP_DIR/AndroidManifest.xml"
RES_DIR="$APP_DIR/res"
ASSETS_DIR="$APP_DIR/assets"
SRC_DIR="$APP_DIR/java"

COMPILED_RES_ZIP="$BUILD_DIR/compiled_res/resources.zip"
UNSIGNED_APK="$BUILD_DIR/aifred-unsigned.apk"
UNALIGNED_APK="$BUILD_DIR/aifred-unaligned.apk"
ALIGNED_APK="$BUILD_DIR/aifred-aligned.apk"
SIGNED_APK="$DIST_DIR/AIFRED-Mobile-debug.apk"

"$AAPT2" compile --dir "$RES_DIR" -o "$COMPILED_RES_ZIP"

"$AAPT2" link \
  -o "$UNSIGNED_APK" \
  -I "$ANDROID_JAR" \
  --manifest "$MANIFEST" \
  -A "$ASSETS_DIR" \
  --java "$BUILD_DIR/gen" \
  --min-sdk-version 26 \
  --target-sdk-version "$TARGET_SDK" \
  "$COMPILED_RES_ZIP"

mapfile -t JAVA_FILES < <(find "$SRC_DIR" "$BUILD_DIR/gen" -name "*.java" -print)
if [ "${#JAVA_FILES[@]}" -eq 0 ]; then
  echo "No Java files found to compile." >&2
  exit 1
fi

javac --release 11 -cp "$ANDROID_JAR" -d "$BUILD_DIR/classes" "${JAVA_FILES[@]}"

mapfile -t CLASS_FILES < <(find "$BUILD_DIR/classes" -name "*.class" -print)
if [ "${#CLASS_FILES[@]}" -eq 0 ]; then
  echo "No compiled classes found for dex conversion." >&2
  exit 1
fi

"$D8" --lib "$ANDROID_JAR" --output "$BUILD_DIR/dex" "${CLASS_FILES[@]}"

cp "$UNSIGNED_APK" "$UNALIGNED_APK"
(cd "$BUILD_DIR/dex" && zip -q "$UNALIGNED_APK" classes.dex)

"$ZIPALIGN" -f -p 4 "$UNALIGNED_APK" "$ALIGNED_APK"

if [ ! -f "$DEBUG_KEYSTORE" ]; then
  mkdir -p "$(dirname "$DEBUG_KEYSTORE")"
  keytool -genkeypair \
    -keystore "$DEBUG_KEYSTORE" \
    -storepass "$STORE_PASS" \
    -alias "$KEY_ALIAS" \
    -keypass "$KEY_PASS" \
    -keyalg RSA \
    -keysize 2048 \
    -validity 10000 \
    -dname "CN=Android Debug,O=Android,C=US"
fi

"$APKSIGNER" sign \
  --ks "$DEBUG_KEYSTORE" \
  --ks-pass "pass:$STORE_PASS" \
  --ks-key-alias "$KEY_ALIAS" \
  --key-pass "pass:$KEY_PASS" \
  --out "$SIGNED_APK" \
  "$ALIGNED_APK"

"$APKSIGNER" verify "$SIGNED_APK"

echo "APK created: $SIGNED_APK"
