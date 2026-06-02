#!/usr/bin/env bash
# Build the modified BlackDex APK locally.
# Prereqs: macOS or Linux, Android SDK with platform 30 + build-tools 30.0.3,
# NDK r21+ installed and on PATH, JDK 11 or 17.
set -euo pipefail

cd "$(dirname "$0")/.."

# Pick a signing config. The repo does not ship a release keystore, so this
# script produces an *unsigned* debug-aligned APK by default. Override with
# KEYSTORE_PATH / KEYSTORE_PASS / KEY_ALIAS to produce a signed release APK.
if [[ -z "${KEYSTORE_PATH:-}" ]]; then
    echo ">> Building debug APK (no keystore supplied)"
    ./gradlew :app:assembleBlackDex32Debug :app:assembleBlackDex64Debug
    OUT_DIR="app/build/outputs/apk"
else
    echo ">> Building signed release APK"
    ./gradlew :app:assembleBlackDex32Release :app:assembleBlackDex64Release \
        -Pandroid.injected.signing.store.file="$KEYSTORE_PATH" \
        -Pandroid.injected.signing.store.password="$KEYSTORE_PASS" \
        -Pandroid.injected.signing.key.alias="$KEY_ALIAS" \
        -Pandroid.injected.signing.key.password="${KEY_PASS:-$KEYSTORE_PASS}"
    OUT_DIR="app/build/outputs/apk"
fi

echo
echo ">> APK output:"
find "$OUT_DIR" -name '*.apk' -print
echo
echo ">> Copying artefacts to ./dist"
mkdir -p dist
find "$OUT_DIR" -name '*.apk' -exec cp -v {} dist/ \;
