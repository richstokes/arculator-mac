My attempt at getting arculator, the Acorn Archimedes emulator, to compile and run successfully on MacOS.  

Right now, it builds and runs on MacOS 12.4.  🎉

There's a few kinks to work out (some of the menu items don't work), but seems to run pretty well!  

&nbsp;


## Usage

Run `./mac-install.sh` -- read through the script first, since it installs a bunch of packages, to make sure you're happy with what it will do.  

Once built, you'll need to put a RiscOS ROM in one of the subdirectories under `./roms`. Name the file `ROM`. 

Run with `./arculator`.
