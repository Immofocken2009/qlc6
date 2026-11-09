#!/bin/bash
# Build qlcplus6 .deb package (parallel-installable with qlcplus/qlcplus5)
# Run this on a Debian/Ubuntu machine with Qt 6.4+ available in apt.
#
# Usage:
#   ./build-deb.sh          # install deps + build
#   ./build-deb.sh nodeps   # skip apt install (deps already installed)

set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

DEPS=(
    debhelper fakeroot dpkg-dev cmake g++ make pkg-config
    qt6-base-dev qt6-declarative-dev qt6-multimedia-dev
    qt6-serialport-dev qt6-websockets-dev qt6-3d-dev
    qt6-tools-dev qt6-tools-dev-tools qt6-svg-dev libxkbcommon-dev
    libfftw3-dev libftdi1-dev libsndfile1-dev
    libasound2-dev libudev-dev libusb-1.0-0-dev
    shared-mime-info libxml2-utils
)

if [ "${1:-}" != "nodeps" ]; then
    echo "==> Installing build dependencies..."
    apt-get update -qq
    apt-get install -y "${DEPS[@]}"
fi

cd "$SCRIPT_DIR"
echo "==> Building qlcplus6 .deb (this takes 1–3 hours)..."
./create-deb.sh v6

echo ""
echo "==> Done. Package(s):"
DEBS=$(find /root -maxdepth 1 -name "qlcplus6_6.0.0*.deb" 2>/dev/null)
if [ -z "$DEBS" ]; then
    DEBS=$(find /root -maxdepth 1 -name "qlcplus_6.0.0*.deb" 2>/dev/null)
fi
echo "$DEBS"
