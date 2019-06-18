# README #

hello os for study


helloos.lds         ld script

helloos.s           boot start file. in the entry point of system.

Makefile            Makefile

README.md           ReadMe file

\"lowinit\_init.s\_\"   low level initialzation

### How to set up in Linux ###

In order to start it your system must support graphics. If you connect remotelly make sure to use ssh -Y.

To compile the project do

$ zypper in -y glibc-devel-32bit
$ make

### How to run ###

$ make run.x86

All targets are listed in buildrules/linker.rules. You can also run it with debugging and for other architectures.
