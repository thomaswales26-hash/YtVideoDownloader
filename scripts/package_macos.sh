#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
APP="$ROOT/build/VMStudio.app"

if [[ ! -d "$APP" ]]; then
    echo "Build the app first: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release ..." >&2
    exit 1
fi

for tool in brew codesign; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Required packaging tool not found: $tool" >&2
        exit 1
    fi
done

QT_PREFIX="$(brew --prefix qt)"
MACDEPLOYQT="$QT_PREFIX/bin/macdeployqt"
if [[ ! -x "$MACDEPLOYQT" ]]; then
    echo "Install Qt with Homebrew (`brew install qt`)." >&2
    exit 1
fi

"$MACDEPLOYQT" "$APP"

codesign --force --deep --sign - "$APP"
codesign --verify --deep --strict "$APP"
echo "Packaged app: $APP"
