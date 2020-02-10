# J1939 decode library

This repository builds j1939decode as a separate static library.

This library by default will attempt to read the file `J1939db.json` from the current directory to load in the J1939 database.
If this file cannot be read, J1939 decoding will not be possible.
This file name and path can changed by redefining the `J1939DB` C macro to point to a different filename.

See below for how to generate the J1939 database file.

## Install

To install run `make` then `sudo make install`.

## Uninstall

To uninstall run `sudo make uninstall`.

## How to link in a Makefile

1. Include the compiler flag `-I/usr/local/include/j1939decode`
1. Include the compiler flag `-L/usr/local/lib`
1. Add the linker flag `-lj1939decode`

## Library usage

Call `j1939_init()` first before calling `j1939_decode_to_json()`.
When done, call `j1939_deinit()` to free memory allocated by `j1939_init()`.

## J1939 database file generation

The `J1939db.json` database file is a JSON formatted file that contains all of the PGN, SPN, and SA lookup data needed for decoding J1939 messages.

This database file can be generated using the `create_j1939db-json.py` script provided by [pretty\_j1939](https://github.com/nmfta-repo/pretty_j1939) repository.
A copy of the [SAE J1939 Digital Annex](https://www.sae.org/standards/content/j1939da_201704/) spreadsheet is required.

For example:

```
create_j1939db-json.py -f J1939DA_201704.xls -w J1939db.json
```
