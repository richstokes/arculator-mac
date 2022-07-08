#!/bin/bash
set -e
ROMFILE='../../../riscos.rom' # PATH to RISOCS ROM file

# Check if running on MacOS
if [ "$(uname)" != "Darwin" ]; then
    echo "This script must be run on a MacOS machine"
    exit 1
fi

# Check for or install homebrew
if test ! $(which brew); then
  echo "Installing homebrew.."
  ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
fi

echo "Installing Mac dependencies.."
brew install sdl2 libx11 gcc automake autoconf cmake libtool hunspell || true 
# export LIBRARY_PATH="/usr/local/lib:/usr/local/Cellar/sdl2:/usr/local/lib:/usr/local/Cellar/sdl2:/opt/homebrew/lib:$(brew info libx11 | grep /usr | cut -d" " -f1)/lib:$(brew info sdl2 | grep /usr | cut -d" " -f1):/usr/local/include/wx-3.1"
# export LD_LIBRARY_PATH=/usr/local/lib
export LDFLAGS="-lX11"
# export C_INCLUDE_PATH=/usr/local/include/wx-3.1
# export CFLAGS="-I/usr/local/include"
# export CXXFLAGS="-I/usr/local/include -I/usr/local/include/wx-3.1"

# check if wxWidgets directory already exists
if [ ! -d "/usr/local/include/wx-3.2" ]; then
    brew uninstall wxwidgets || true # Install wxwidgets from source (the brew version doesn't seem to work)
    wget https://github.com/wxWidgets/wxWidgets/releases/download/v3.2.0/wxWidgets-3.2.0.tar.bz2
    tar -xjf wxWidgets-3.2.0.tar.bz2 > /dev/null
    cd wxWidgets-3.2.0
    ./configure
    echo "Building wxwidgets, this may take a while.."
    make -j$(nproc) > /dev/null
    echo ""
    echo "Installing wxWidgets, you may be prompted for your password.."
    sudo make install
    cd -
    #TODO: just extract the wx dir under ./src??
fi


# check if libtoolize exists
if [ -f "/usr/local/bin/libtoolize" ]; then
    echo "libtoolize already installed."
else
    echo "Linking glibtoolize to libtoolize.."
    ln -s /usr/local/bin/glibtoolize /usr/local/bin/libtoolize
fi

autoreconf -i #Fixes "configure: error: cannot find install-sh, install.sh"

echo "Building arculator.."
make clean || true

# Swap the two lines below if you want to use the debug version
# ./configure --enable-release-build
./configure --enable-debug

make -j$(nproc)

# Copy your RiscOS rom to the arculator ROMs folder
# Check ROMFILE exists

if [ -f "$ROMFILE" ]; then
    echo "Copying $ROMFILE to arculator/roms dir.."
    cp $ROMFILE roms/riscos310/ROM
else
    echo "ROMFILE $ROMFILE not found.."
fi

echo ""
echo "Build completed!\n Use ./arculator to run"