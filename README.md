# Overview

My modifications to arculator, the Acorn Archimedes emulator, allowing it to compile and run successfully on macOS on Apple Silicon.  

Right now, it builds and runs on macOS Tahoe.  🎉

There's a few kinks to work out (some of the menu items don't work), but seems to run pretty well!  

&nbsp;

## Build & Run

Run `./mac-install.sh` -- read through the script first, since it installs a bunch of packages, and will ask you to confirm before installing anything.  

Once built, you'll need to put a RiscOS ROM in one of the subdirectories under `./roms`. Name the file `ROM`.

Run with `./arculator`.
