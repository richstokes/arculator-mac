My attempt at getting arculator to compile and run successfully on MacOS.  

Try `./mac-install.sh` -- read through the script first, since it installs a bunch of packages, to make sure you're happy with what it will do.  


&nbsp;

Currently compiles, but gives a `SDL_CreateWindowFrom could not be created! Error: NSWindow drag regions should only be invalidated on the Main Thread!` error when trying to start the emulator. Seems to be something with how MacOS expects graphics threads to be handled.