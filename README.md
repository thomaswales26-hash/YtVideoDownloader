# VM Studio

VM Studio is a macOS desktop frontend for QEMU. It creates a virtual disk, launches a Linux or Windows installer ISO, and saves your machine settings between launches.

## Requirements

- macOS and Qt 6.4 or newer
- CMake 3.21 or newer
- QEMU, including its UEFI firmware files
- A Linux or Windows installer ISO that matches the selected guest architecture

Install the build tools, QEMU, and TPM emulator with Homebrew:

```sh
brew install cmake qt qemu swtpm
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build
open build/VMStudio.app
```

Choose an ISO, set the machine resources and disk capacity, then select **Create & Start**. For Windows, TPM 2.0 and Secure Boot are enabled by default and can be toggled in the machine settings. TPM state is saved beside the virtual disk; UEFI Secure Boot variables are copied from the installed firmware template once and persist beside the disk. The app opens QEMU's display in a separate window. Once the operating system is installed, uncheck **Boot from installer ISO** before starting the VM again.

On Apple Silicon, ARM64 guests use Apple's Hypervisor framework; x86_64 guests use software emulation and will be slower. The guest ISO must match the selected architecture. Secure Boot requires matching Secure Boot-capable EDK2/OVMF firmware code and variable-template files from QEMU; the app reports an error if it cannot find them. Windows 11 still requires an installer and hardware configuration supported by the selected guest architecture.

QEMU is discovered from `PATH`, `/opt/homebrew/bin`, or `/usr/local/bin` and is not bundled with the app. Installer ISOs and virtual disks are user-provided; make sure you have the right to use them and enough free host storage.

## macOS packaging

```sh
bash scripts/package_macos.sh
bash scripts/create_installer_pkg.sh
```

The installer is written to `build/VMStudio-1.0.1.pkg` and installs the app to `/Applications`. It is unsigned; distribution outside your own machine requires signing and notarization with an Apple Developer ID.
