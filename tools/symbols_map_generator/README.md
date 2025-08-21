# Symbols Map Generator
This tool parses the header files of a library in the Mir project to extract symbols.
These symbols are outputted to a corresponding `symbols.map` file by default. If the `--diff`
option is provided, then the added and removed symbols will be printed to `stdout`.

## Set Up
Please see [How to Update Symbols Map
Files](https://canonical-mir.readthedocs-hosted.com/stable/how-to/how-to-update-symbols-map/)

## Usage
The following will output the introduced symbols to `stdout` for version 5.0 of the `miral` library:
```sh
./main.py --library miral --version 5.0 --diff
```

The following will write the introduced symbols to `src/miral/symbols.map` for version 5.0 of the `miral` library:
```sh
./main.py --library miral --version 5.0 --output_symbols
```

## Requirements
- `libclang-dev` from apt
- The `clang` python package
