#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DESTINATION="${1:-$ROOT/build/VMStudio.app/Contents/Resources/firmware}"
DEBIAN_EDK2_VERSION="2026.08+ds-3"
OVMF_SHA256="0ee404cbf543ce0077c159a946fa8012b779b94a8c158678dc94f290fbd67eea"
AAVMF_SHA256="6a022fbdc7dbac1336888d5cce00d9430eb49ade465418ae896ac326553d7809"

for tool in curl shasum ar tar; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Required firmware packaging tool not found: $tool" >&2
        exit 1
    fi
done

temporary_directory="$(mktemp -d)"
trap 'rm -rf "$temporary_directory"' EXIT

ovmf_package="$temporary_directory/ovmf.deb"
aavmf_package="$temporary_directory/aavmf.deb"
curl --location --fail --silent --show-error \
    "https://deb.debian.org/debian/pool/main/e/edk2/ovmf-generic_${DEBIAN_EDK2_VERSION}_all.deb" \
    -o "$ovmf_package"
curl --location --fail --silent --show-error \
    "https://deb.debian.org/debian/pool/main/e/edk2/qemu-efi-aarch64_${DEBIAN_EDK2_VERSION}_all.deb" \
    -o "$aavmf_package"

echo "$OVMF_SHA256  $ovmf_package" | shasum --check --status || {
    echo "Debian OVMF package checksum verification failed." >&2
    exit 1
}
echo "$AAVMF_SHA256  $aavmf_package" | shasum --check --status || {
    echo "Debian AAVMF package checksum verification failed." >&2
    exit 1
}

mkdir -p "$temporary_directory/x86_64" "$temporary_directory/aarch64"
(cd "$temporary_directory/x86_64" &&
    ar x "$ovmf_package" data.tar.xz &&
    tar -xJf data.tar.xz -C . \
        ./usr/share/OVMF/OVMF_CODE_4M.secboot.fd \
        ./usr/share/OVMF/OVMF_VARS_4M.ms.fd \
        ./usr/share/doc/ovmf-generic/copyright)
(cd "$temporary_directory/aarch64" &&
    ar x "$aavmf_package" data.tar.xz &&
    tar -xJf data.tar.xz -C . \
        ./usr/share/AAVMF/AAVMF_CODE.secboot.fd \
        ./usr/share/AAVMF/AAVMF_VARS.ms.fd \
        ./usr/share/doc/qemu-efi-aarch64/copyright)

x86_code="$temporary_directory/x86_64/usr/share/OVMF/OVMF_CODE_4M.secboot.fd"
x86_vars="$temporary_directory/x86_64/usr/share/OVMF/OVMF_VARS_4M.ms.fd"
arm_code="$temporary_directory/aarch64/usr/share/AAVMF/AAVMF_CODE.secboot.fd"
arm_vars="$temporary_directory/aarch64/usr/share/AAVMF/AAVMF_VARS.ms.fd"
for firmware in "$x86_code" "$x86_vars" "$arm_code" "$arm_vars"; do
    if [[ ! -s "$firmware" ]]; then
        echo "Expected firmware file is missing or empty: $firmware" >&2
        exit 1
    fi
done
for certificate in \
    "Microsoft UEFI CA 2023" \
    "Windows UEFI CA 2023" \
    "Microsoft Corporation KEK 2K CA 2023"; do
    if ! grep -aFq "$certificate" "$x86_vars" || ! grep -aFq "$certificate" "$arm_vars"; then
        echo "The firmware templates do not contain the expected Windows Secure Boot certificate: $certificate" >&2
        exit 1
    fi
done

mkdir -p "$DESTINATION/x86_64" "$DESTINATION/aarch64"
install -m 644 "$x86_code" "$DESTINATION/x86_64/OVMF_CODE.secboot.fd"
install -m 644 "$x86_vars" "$DESTINATION/x86_64/OVMF_VARS.ms.fd"
install -m 644 "$temporary_directory/x86_64/usr/share/doc/ovmf-generic/copyright" \
    "$DESTINATION/x86_64/LICENSE"
install -m 644 "$arm_code" "$DESTINATION/aarch64/AAVMF_CODE.secboot.fd"
install -m 644 "$arm_vars" "$DESTINATION/aarch64/AAVMF_VARS.ms.fd"
install -m 644 "$temporary_directory/aarch64/usr/share/doc/qemu-efi-aarch64/copyright" \
    "$DESTINATION/aarch64/LICENSE"

echo "Bundled Debian EDK2 Secure Boot firmware with Microsoft UEFI 2023 keys in: $DESTINATION"
