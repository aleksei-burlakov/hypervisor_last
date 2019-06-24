# README #

hello os for study


helloos.lds         ld script

helloos.s           boot start file. in the entry point of system.

Makefile            Makefile

README.md           ReadMe file

\"lowinit\_init.s\_\"   low level initialzation

All targets are listed in buildrules/linker.rules. You can also run it with debugging and for other architectures.
=======
* Require Packages
  * openSUSE TW

* Build

~~~~
  `make`
~~~~

* Run Hypervisor

~~~~
  `make run.x86`
~~~~

### How to set up in Linux ###

In order to start it your system must support graphics. If you connect remotelly make sure to use ssh -Y.

To compile the project do

$ zypper in -y glibc-devel-32bit
$ make

### How to run ###

$ make run.x86

### TODO ###

1. memory management, for now, I use "segment", not use "page", because I don't very familiar with it and the last hackweek I did some else project. But It must be used in the future. 
2. Interrupt of the timer, it is basic of the scheduler, but due to we need a good memory module design. so might be it is hard to integrate into our code.