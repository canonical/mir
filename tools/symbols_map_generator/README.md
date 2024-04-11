# Symbols Map Generator
This tool parses the header files of a library in the Mir project to extract symbols.
These symbols can then either be outputted to `stdout` to display a diff or to the
corresponding `symbols.map` file.

## Usage
It is recommended that the script be run from a python venv:
```sh
python -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

The following will output the introduced symbols to `stdout` for version 5.0 of the `miral` library:
```sh
./main.py --library miral --version 5.0 --diff
```

The following will write the introduced symbols to `src/miral/symbols.map` for version 5.0 of the `miral` library:
```sh
./main.py --library miral --version 5.0 --output_symbols
```

## Requirements
- `libclang-dev`
- `pip install libclang`
