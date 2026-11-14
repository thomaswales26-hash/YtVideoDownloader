# Video Downloader

A small macOS desktop frontend for yt-dlp, built with Qt Widgets and the Fusion style. It accepts URLs from sites supported by yt-dlp; no downloader can support every website. Downloads prefer up to 4K (2160p), including 1080p when available, and fall back to a site's best available format when necessary. Higher resolutions use more storage and bandwidth.

## Build and package

- macOS with Qt 6.4 or newer (Qt Widgets)
- CMake 3.21 or newer
- Homebrew

```sh
brew install cmake qt ffmpeg
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build
bash scripts/package_macos.sh
bash scripts/create_installer_pkg.sh
```

The package script bundles Qt, FFmpeg and its required libraries, plus the checksum-verified standalone yt-dlp macOS release. The app targets native Apple Silicon (arm64).

The installer is created at `build/VideoDownloader-1.0-arm64.pkg` and installs the app to `/Applications`. It is unsigned; macOS distribution outside your own machine requires signing and notarization with an Apple Developer ID.

Launch a locally built app with `open build/VideoDownloader.app`.

Use the app only for content you are authorized to download, and follow the source site's terms. The app does not bypass DRM or access restrictions.
