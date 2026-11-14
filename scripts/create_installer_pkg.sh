#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
APP="$ROOT/build/VMStudio.app"
PKG="$ROOT/build/VMStudio-1.0.2.pkg"

if [[ ! -d "$APP" ]]; then
    echo "Build and package the app first; see README.md." >&2
    exit 1
fi

codesign --verify --deep --strict "$APP"
pkgbuild \
    --component "$APP" \
    --install-location /Applications \
    --identifier com.example.vmstudio \
    --version 1.0.2 \
    --ownership recommended \
    "$PKG"

echo "Installer package: $PKG"
