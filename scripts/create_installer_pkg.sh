#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
APP="$ROOT/build/VideoDownloader.app"
PKG="$ROOT/build/VideoDownloader-1.0-arm64.pkg"

if [[ ! -d "$APP" ]]; then
    echo "Build and package the app first; see README.md." >&2
    exit 1
fi

codesign --verify --deep --strict "$APP"
pkgbuild \
    --component "$APP" \
    --install-location /Applications \
    --identifier com.example.videodownloader \
    --version 1.0 \
    --ownership recommended \
    "$PKG"

echo "Installer package: $PKG"
