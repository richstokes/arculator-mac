#!/bin/bash
set -e
ROMFILE='../../../riscos.rom' # PATH to RISOCS ROM file

# Check for or install homebrew
if test ! $(which brew); then
  echo "Installing homebrew.."
  ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
fi

echo "Installing Mac dependencies.."
brew install sdl2 libx11 gcc automake autoconf cmake libtool hunspell || true 
# export LIBRARY_PATH="/usr/local/lib:/usr/local/Cellar/sdl2:/usr/local/lib:/usr/local/Cellar/sdl2:/opt/homebrew/lib:$(brew info libx11 | grep /usr | cut -d" " -f1)/lib:$(brew info sdl2 | grep /usr | cut -d" " -f1):/usr/local/include/wx-3.1"
# export LD_LIBRARY_PATH=/usr/local/lib
# export LDFLAGS="-lX11"
# export C_INCLUDE_PATH=/usr/local/include/wx-3.1
# export CFLAGS="-I/usr/local/include"
# export CXXFLAGS="-I/usr/local/include -I/usr/local/include/wx-3.1"
echo ""
echo "Library path=$LIBRARY_PATH"
echo ""

# check if wxWidgets directory already exists
if [ ! -d "/usr/local/include/wx-3.1" ]; then
    # Install wxwidgets from source (the brew version doesn't seem to work)
    brew uninstall wxwidgets || true
    wget https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.7/wxWidgets-3.1.7.tar.bz2
    tar -xjf wxWidgets-3.1.7.tar.bz2 > /dev/null
    cd wxWidgets-3.1.7
    ./configure
    echo "Building wxwidgets, this may take a while.."
    make -j8 > /dev/null
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
make clean
./configure --enable-release-build
make -j8

# Copy your RiscOS rom to the arculator ROMs folder
cp $ROMFILE roms/riscos310/ROM