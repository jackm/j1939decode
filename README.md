# J1939 decode library

Decode J1939 CAN bus messages into a JSON string representation of what the payload data means based on the SAE standard.

This library by default will attempt to read the file `J1939db.json` from the current directory to load in the J1939 database.
If this file cannot be read, J1939 decoding will not be possible.
This file name and path can changed by redefining the `J1939DB` C macro to point to a different filename.

***See below for how to generate the J1939 database file.***

## Building

Create a build directory relative to project root, run cmake and then make.

```
mkdir -p build
cd build
cmake ..
make -j
```

## Cleaning

Remove generated directories: `build/`

## Install

To install first build the library, then run `sudo make install` from within the build directory.

## Uninstall

To uninstall run `sudo make uninstall` from within the build directory.

## Testing

Unit tests make use of the [Unity testing framework](http://www.throwtheswitch.org/unity) which is used by the [Ceedling](http://www.throwtheswitch.org/ceedling) build system.

To start all tests run `ceedling test:all`.

## Library usage

Call `j1939_init()` first before calling `j1939_decode_to_json()`.

When done, call `j1939_deinit()` to free memory allocated by `j1939_init()` and free the string pointer returned by `j1939_decode_to_json()`.

## J1939 database file generation

The `J1939db.json` database file is a JSON formatted file that contains all of the PGN, SPN, and SA lookup data needed for decoding J1939 messages.

This database file can be generated using the `create_j1939db-json.py` script provided by [pretty\_j1939](https://github.com/nmfta-repo/pretty_j1939) repository.
A copy of the [SAE J1939 Digital Annex](https://www.sae.org/standards/content/j1939da_201704/) spreadsheet is required.

For example:

```
create_j1939db-json.py -f J1939DA_201704.xls -w J1939db.json
```
