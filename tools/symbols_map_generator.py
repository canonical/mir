#!/usr/bin/env python3

# Copyright © Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 or 3
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import logging
import glob
import os
import argparse
from pathlib import Path
from typing import Literal, TypedDict, get_args
import clang.cindex

_logger = logging.getLogger(__name__)

LibraryName = Literal[
    "miral", "mir_common", "mir_core", "miroil", "mir_platform",
    "mir_renderer", "mir_renderers", "mir_server", "mir_test", "mir_wayland"]


class LibraryInfo(TypedDict):
    external_headers: list[str]
    internal_headers: list[str]
    external_output_file: str
    internal_output_file: str


# Directory paths are relative to the root of the Mir project
library_to_data_map: dict[LibraryName, LibraryInfo] = {
    "miral": {
        "external_headers": [
            "include/miral"
        ],
        "internal_headers": [
            "src/miral"
        ],
        "external_output_file": "src/miral/symbols_external.map",
        "internal_output_file": "src/miral/symbols_internal.map"
    }
}


def get_absolute_path_form_project_path(project_path: str) -> str:
    script_path = os.path.dirname(os.path.realpath(__file__))
    path = Path(script_path).parent.joinpath(project_path)
    return path.absolute().as_posix()


def is_operator_overload(spelling: str):
    operators = [
        "+",
        "-",
        "*",
        "/",
        "%",
        "^",
        "&",
        "|",
        "~",
        "!",
        "=",
        "<",
        ">",
        "+=",
        "-=",
        '*=',
        "/=",
        "%=",
        "^=",
        "&=",
        "|=",
        "<<",
        ">>",
        ">>=",
        "<<=",
        "==",
        "!=",
        "<=",
        ">=", 
        "<=>",
        "&&",
        "||",
        "++",
        "--",
        ",",
        "->*",
        "->",
        "()",
        "[]"
    ]

    if spelling.startswith("operator "):
        return True

    for op in operators:
        if spelling == f"operator{op}":
            return True
        
    return False


def create_symbol_name(node: clang.cindex.Cursor) -> str:
    if(node.kind == clang.cindex.CursorKind.CLASS_DECL
        or node.kind == clang.cindex.CursorKind.STRUCT_DECL):
        return node.displayname
    
    if node.kind == clang.cindex.CursorKind.DESTRUCTOR:
        return f"?{node.spelling[1:]}*"

    if ((node.kind == clang.cindex.CursorKind.FUNCTION_DECL 
        or node.kind == clang.cindex.CursorKind.CXX_METHOD)
        and is_operator_overload(node.spelling)):
        return "operator*"

    return f"{node.spelling}*"


def should_generate_as_class_or_struct(node: clang.cindex.Cursor):
    return ((node.kind == clang.cindex.CursorKind.CLASS_DECL
          or node.kind == clang.cindex.CursorKind.STRUCT_DECL)
          and node.is_definition())


def traverse_ast(node: clang.cindex.Cursor, filename: str, result: set[str]) -> set[str]:
    # Ignore private and protected variables
    if (node.access_specifier == clang.cindex.AccessSpecifier.PRIVATE):
        return result
    
    # Check if we need to output a symbol
    if ((node.kind == clang.cindex.CursorKind.FUNCTION_DECL 
          or node.kind == clang.cindex.CursorKind.CXX_METHOD
          or node.kind == clang.cindex.CursorKind.VAR_DECL
          or node.kind == clang.cindex.CursorKind.CONSTRUCTOR
          or node.kind == clang.cindex.CursorKind.DESTRUCTOR
          or node.kind == clang.cindex.CursorKind.ENUM_DECL
          or node.kind == clang.cindex.CursorKind.FIELD_DECL
          or should_generate_as_class_or_struct(node))
          and node.location.file.name == filename
          and not node.is_pure_virtual_method()):
        parent = node.lexical_parent
        namespace_str = create_symbol_name(node)

        # Walk up the tree to build the namespace
        while parent is not None:
            if parent.kind == clang.cindex.CursorKind.TRANSLATION_UNIT:
                break

            namespace_str = f"{parent.displayname}::{namespace_str}"
            parent = parent.lexical_parent

        # Classes and structs have a specific outpu
        if (node.kind == clang.cindex.CursorKind.CLASS_DECL
            or node.kind == clang.cindex.CursorKind.STRUCT_DECL):
            result.add(f"vtable?for?{namespace_str};")
            result.add(f"typeinfo?for?{namespace_str};")
        else:
            result.add(f"{namespace_str};")
            
            # Check if we're marked virtual
            if ((node.kind == clang.cindex.CursorKind.CXX_METHOD
                or node.kind == clang.cindex.CursorKind.DESTRUCTOR
                or node.kind == clang.cindex.CursorKind.CONSTRUCTOR)
                and node.is_virtual_method()):
                result.add(f"non-virtual?thunk?to?{namespace_str};")
            else:
                # Check if we're marked override
                for  child in node.get_children():
                    if child.kind == clang.cindex.CursorKind.CXX_OVERRIDE_ATTR:
                        result.add(f"non-virtual?thunk?to?{namespace_str};")
                        break

    # Traverse down the tree if we can
    if (node.kind == clang.cindex.CursorKind.TRANSLATION_UNIT
        or node.kind == clang.cindex.CursorKind.CLASS_DECL
        or node.kind == clang.cindex.CursorKind.STRUCT_DECL
        or node.kind == clang.cindex.CursorKind.NAMESPACE):
        for child in node.get_children():
            traverse_ast(child, filename, result)

    return result


def process_directory(directory: str) -> list[str]:
    current_set = set()

    files_dir = get_absolute_path_form_project_path(directory)
    _logger.debug(f"Processing external directory: {files_dir}")
    files = glob.glob(files_dir + '/**/*.h', recursive=True)
    for file in files:
        idx = clang.cindex.Index.create()
        tu = idx.parse(
            file, 
            args=['-std=c++20', '-x', 'c++-header', '-nostdlibinc'],  
            options=0)
        root = tu.cursor
        list(traverse_ast(root, file, current_set))

    result = list(current_set)
    result.sort()
    return result 


def output_symbols_to_file(symbols: list[str], library: str, version: str, output_file: str):
    joint_symbols = "\n    ".join(symbols)
    output_str = f'''{library.upper()}_{version} {"{"}
global:
  extern "C++" {"{"}
    {joint_symbols}
  {'};'}
local: *;
{'}'};'''
    
    output_path = get_absolute_path_form_project_path(output_file)
    with open(output_path, "w") as f:
        f.write(output_str)
        return


def main():
    parser = argparse.ArgumentParser(description="This tool generates symbols.map files for the Mir project. "
                                        "It uses https://pypi.org/project/libclang/ to process "
                                        "the AST of the project's header files.")
    parser.add_argument('--library', metavar='-l', type=str,
                    help='library to generate symbols for',
                    required=True,
                    choices=list(get_args(LibraryName)))
    parser.add_argument('--symbol-version', metavar='-s', type=str,
                    help='version to generate symbols for (e.g. 5.0)',
                    required=True)
    
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO)
    _logger.debug("Symbols map generation is beginning")

    library = args.library
    try:
        directories: LibraryInfo = library_to_data_map[library]
    except KeyError:
        _logger.error(f"The provided library has yet to be implmented: {library}")
        exit(1)

    _logger.debug(f"Generating symbols for {library}")

    # First, we process the external symbols
    external_symbols = []
    for directory in directories["external_headers"]:
        external_symbols += process_directory(directory)
    output_symbols_to_file(
        external_symbols,
        library,
        args.symbol_version,
        directories["external_output_file"])
    
    # Then, we process the internal symbols
    internal_symbols = []
    for directory in directories["internal_headers"]:
        internal_symbols += process_directory(directory)
    output_symbols_to_file(
        internal_symbols,
        library,
        args.symbol_version,
        directories["internal_output_file"])
    exit(0)

    

if __name__ == "__main__":
    main()
