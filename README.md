# VM Studio

VM Studio is a macOS desktop frontend for QEMU. It creates a virtual disk, launches a Linux or Windows installer ISO, and saves your machine settings between launches.

## Requirements

- macOS and Qt 6.4 or newer
- CMake 3.21 or newer
- QEMU and `swtpm`
- A Linux or Windows installer ISO that matches the selected guest architecture

Install the build tools, QEMU, and TPM emulator with Homebrew:

```sh
brew install cmake qt qemu swtpm
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build
bash scripts/package_macos.sh
open build/VMStudio.app
```

Choose an ISO, set the machine resources and disk capacity, then select **Create & Start**. For Windows, TPM 2.0 and Secure Boot are enabled by default and can be toggled in the machine settings. TPM state is saved beside the virtual disk; UEFI Secure Boot variables are copied from the installed firmware template once and persist beside the disk. The app opens QEMU's display in a separate window. Once the operating system is installed, uncheck **Boot from installer ISO** before starting the VM again.

On Apple Silicon, ARM64 guests use Apple's Hypervisor framework; x86_64 guests use software emulation and will be slower. The guest ISO must match the selected architecture. Secure Boot-capable EDK2 firmware for x86_64 and ARM64, with Microsoft 2023 Secure Boot certificates enrolled, is bundled into the macOS app from Debian's `ovmf-generic` and `qemu-efi-aarch64` packages. Debian copyright and license notices are included with each firmware pair. `scripts/bundle_secure_boot_firmware.sh` downloads pinned Debian packages, verifies their SHA-256 checksums, and checks for the Microsoft Windows UEFI CA 2023 certificates before bundling. Windows 11 still requires an installer and hardware configuration supported by the selected guest architecture.

QEMU is discovered from `PATH`, `/opt/homebrew/bin`, or `/usr/local/bin` and is not bundled with the app. Installer ISOs and virtual disks are user-provided; make sure you have the right to use them and enough free host storage.

## macOS packaging

```sh
bash scripts/package_macos.sh
bash scripts/create_installer_pkg.sh
```

The installer is written to `build/VMStudio-1.0.4.pkg` and installs the app to `/Applications`. It is unsigned; distribution outside your own machine requires signing and notarization with an Apple Developer ID.
