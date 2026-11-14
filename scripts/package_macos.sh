#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
APP="$ROOT/build/VideoDownloader.app"
TOOLS="$APP/Contents/MacOS/tools"
YT_DLP_VERSION="2026.10.26"
YT_DLP_SHA256="0f922c15ccc0b83eefa1c8bc9104e434318c4bf16e408e1ee93b7745a6efe0aa"

if [[ ! -d "$APP" ]]; then
    echo "Build the app first: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release ..." >&2
    exit 1
fi

for tool in brew curl shasum otool install_name_tool codesign; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Required packaging tool not found: $tool" >&2
        exit 1
    fi
done

QT_PREFIX="$(brew --prefix qt)"
FFMPEG="$(brew --prefix ffmpeg)/bin/ffmpeg"
MACDEPLOYQT="$QT_PREFIX/bin/macdeployqt"
if [[ ! -x "$FFMPEG" || ! -x "$MACDEPLOYQT" ]]; then
    echo "Install Qt and FFmpeg with Homebrew (`brew install qt ffmpeg`)." >&2
    exit 1
fi

mkdir -p "$TOOLS"
rm -f "$TOOLS/yt-dlp"
"$QT_PREFIX/bin/macdeployqt" "$APP"
cp "$FFMPEG" "$TOOLS/ffmpeg"
"$MACDEPLOYQT" "$APP" "-executable=$TOOLS/ffmpeg" -no-plugins -always-overwrite

for library in "$APP"/Contents/Frameworks/*.dylib; do
    [[ -e "$library" ]] || continue
    while IFS= read -r dependency; do
        name="${dependency##*/}"
        install_name_tool -change "$dependency" "@loader_path/$name" "$library"
    done < <(otool -L "$library" | awk '/@executable_path\/\.\.\/Frameworks\// {print $1}')
done

temporary_directory="$(mktemp -d)"
yt_dlp_download="$temporary_directory/yt-dlp"
trap 'rm -f "$yt_dlp_download"; rmdir "$temporary_directory"' EXIT
yt_dlp_url="https://github.com/yt-dlp/yt-dlp/releases/download/$YT_DLP_VERSION/yt-dlp_macos"
curl --location --fail --silent --show-error "$yt_dlp_url" -o "$yt_dlp_download"
echo "$YT_DLP_SHA256  $yt_dlp_download" | shasum --check --status || {
    echo "yt-dlp checksum verification failed." >&2
    exit 1
}
install -m 755 "$yt_dlp_download" "$TOOLS/yt-dlp"

codesign --force --deep --sign - "$APP"
codesign --verify --deep --strict "$APP"
"$TOOLS/yt-dlp" --version
"$TOOLS/ffmpeg" -version | head -n 1
echo "Packaged app: $APP"
