# Overview

My modifications to [arculator](https://github.com/sarah-walker-pcem/arculator), the Acorn Archimedes emulator, allowing it to compile and run successfully on macOS with Apple Silicon.  

As of March 2026, confirmed it builds and runs on macOS Tahoe.

&nbsp;

## Build & Run

Run `./mac-install.sh` -- read through the script first, since it installs a bunch of packages, and will ask you to confirm before installing anything.  

Once built, you'll need to put a RiscOS ROM in one of the subdirectories under `./roms`. Name the file `ROM`.

Run with `./arculator`.
