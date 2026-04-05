#!/bin/bash
set -e

# Parse flags
AUTO_YES=false
while getopts "y" opt; do
    case $opt in
        y) AUTO_YES=true ;;
    esac
done

confirm() {
    if $AUTO_YES; then return 0; fi
    read -p "$1 [y/N] " yn
    [[ "$yn" =~ ^[Yy]$ ]]
}

# Check if running on macOS
if [ "$(uname)" != "Darwin" ]; then
    echo "This script must be run on macOS"
    exit 1
fi

# Detect architecture and set Homebrew prefix
ARCH=$(uname -m)
if [ "$ARCH" = "arm64" ]; then
    BREW_PREFIX="/opt/homebrew"
else
    BREW_PREFIX="/usr/local"
fi
export PATH="$BREW_PREFIX/bin:$PATH"

# Install Xcode command line tools
if ! xcode-select -p &>/dev/null; then
    read -p "Xcode command line tools are required. Install now? [y/N] " yn
    if [[ "$yn" =~ ^[Yy]$ ]]; then
        xcode-select --install
        echo "Please re-run this script after Xcode tools finish installing."
        exit 0
    else
        echo "Cannot continue without Xcode tools."
        exit 1
    fi
fi

# Check for or install Homebrew
if ! command -v brew &>/dev/null; then
    read -p "Homebrew is required but not installed. Install now? [y/N] " yn
    if [[ "$yn" =~ ^[Yy]$ ]]; then
        HOMEBREW_URL="https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh"
        /bin/bash -c "$(curl -fsSL $HOMEBREW_URL)"
    else
        echo "Cannot continue without Homebrew."
        exit 1
    fi
fi

# Install dependencies
PACKAGES="sdl2 automake autoconf libtool pkg-config wxwidgets"
MISSING=""
for pkg in $PACKAGES; do
    if ! brew list "$pkg" &>/dev/null; then
        MISSING="$MISSING $pkg"
    fi
done
if [ -n "$MISSING" ]; then
    read -p "Need to install:$MISSING. Continue? [y/N] " yn
    if [[ "$yn" =~ ^[Yy]$ ]]; then
        brew install $MISSING
    else
        echo "Cannot continue without dependencies."
        exit 1
    fi
fi

# Ensure glibtoolize is available as libtoolize
if ! command -v libtoolize &>/dev/null && command -v glibtoolize &>/dev/null; then
    echo "Linking glibtoolize -> libtoolize"
    ln -sf "$(which glibtoolize)" "$BREW_PREFIX/bin/libtoolize"
fi

# Find wxWidgets m4 macros for aclocal
WX_M4_DIR=$(find "$BREW_PREFIX/Cellar/wxwidgets" -name "wxwin.m4" -exec dirname {} \; 2>/dev/null | head -1)
ACLOCAL_ARGS=""
if [ -n "$WX_M4_DIR" ]; then
    ACLOCAL_ARGS="-I $WX_M4_DIR"
fi

# Run autotools
echo "Running autoreconf..."
autoreconf -i $ACLOCAL_ARGS

# Configure and build
echo "Configuring..."
make clean 2>/dev/null || true
./configure --enable-release-build

echo "Building arculator..."
make -j$(sysctl -n hw.ncpu)

echo ""
echo "Build completed! Run ./arculator to start."
