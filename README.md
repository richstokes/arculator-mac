# Overview

My modifications to [arculator](https://github.com/sarah-walker-pcem/arculator), the Acorn Archimedes emulator, allowing it to compile and run successfully on macOS with Apple Silicon.  

As of March 2026, confirmed it builds and runs on macOS Tahoe.

&nbsp;

## Build & Run

Clone the repo and run `./mac-install.sh` -- The script installs a number of packages, and will ask you to confirm interactively before installing anything.  

Once built, you'll need to put a RiscOS ROM in one of the subdirectories under `./roms`.

Run with `./arculator`.
