# J1939 decode library

This repository builds j1939decode as a separate static library.

## Install

To install run `make` then `sudo make install`.

## Uninstall

To uninstall run `sudo make uninstall`.

## How to link in a Makefile

1. Include the compiler flag `-I/usr/local/include/j1939decode`
1. Include the compiler flag `-L/usr/local/lib`
1. Add the linker flag `-lj1939decode`
