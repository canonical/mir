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
from typing import Literal, TypedDict, get_args, Optional
import clang.cindex

_logger = logging.getLogger(__name__)

LibraryName = Literal[
    "miral", "mir_common", "mir_core", "miroil", "mir_platform",
    "mir_renderer", "mir_renderers", "mir_server", "mir_test", "mir_wayland"]


class LibraryInfo(TypedDict):
    external_headers: list[str]
    internal_headers: Optional[list[str]]
    external_output_file: str
    internal_output_file: Optional[str]
    search_headers: Optional[list[str]]


# Directory paths are relative to the root of the Mir project
library_to_data_map: dict[LibraryName, LibraryInfo] = {
    "miral": {
        "external_headers": [
            "include/miral"
        ],
        "external_output_file": "src/miral/symbols.map",
    },
    "mir_server": {
        "external_headers": [
            "include/server/mir",
            "src/include/server/mir"
        ],
        "external_output_file": "src/server/symbols.map",
        "search_headers": [
            "src/include/server"
        ]
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
        or node.kind == clang.cindex.CursorKind.STRUCT_DECL
        or node.kind == clang.cindex.CursorKind.FIELD_DECL):
        return node.displayname
    
    if node.kind == clang.cindex.CursorKind.DESTRUCTOR:
        return f"?{node.spelling[1:]}*"
    
    if ((node.kind == clang.cindex.CursorKind.FUNCTION_DECL 
        or node.kind == clang.cindex.CursorKind.CXX_METHOD
        or node.kind == clang.cindex.CursorKind.CONVERSION_FUNCTION)
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
          or node.kind == clang.cindex.CursorKind.CONVERSION_FUNCTION
          or should_generate_as_class_or_struct(node))
          and node.location.file.name == filename
          and not node.is_pure_virtual_method()):
        parent = node.semantic_parent
        namespace_str = create_symbol_name(node)

        # Walk up the tree to build the namespace
        while parent is not None:
            if parent.kind == clang.cindex.CursorKind.TRANSLATION_UNIT:
                break

            namespace_str = f"{parent.spelling}::{namespace_str}"
            parent = parent.semantic_parent

        # Classes and structs have a specific outpu
        if (node.kind == clang.cindex.CursorKind.CLASS_DECL
            or node.kind == clang.cindex.CursorKind.STRUCT_DECL):
            result.add(f"vtable?for?{namespace_str};")
            result.add(f"typeinfo?for?{namespace_str};")
        else:
            result.add(f"{namespace_str};")
            
            # Check if we're marked virtual
            is_virtual = False
            if ((node.kind == clang.cindex.CursorKind.CXX_METHOD
                or node.kind == clang.cindex.CursorKind.DESTRUCTOR
                or node.kind == clang.cindex.CursorKind.CONSTRUCTOR
                or node.kind == clang.cindex.CursorKind.CONVERSION_FUNCTION)
                and node.is_virtual_method()):
                result.add(f"non-virtual?thunk?to?{namespace_str};")
                is_virtual = True
            else:
                # Check if we're marked override
                for  child in node.get_children():
                    if child.kind == clang.cindex.CursorKind.CXX_OVERRIDE_ATTR:
                        result.add(f"non-virtual?thunk?to?{namespace_str};")
                        is_virtual = True
                        break

            # If this is a virtual/override, then we iterate the virtual base
            # classes.. If we find find the provided method in those classes
            # we need to generate a a virtual?thunk?to? for the method/constructor/deconstructor.
            if is_virtual:
                def search_class_hierarchy_for_virtual_thunk(derived: clang.cindex.Cursor):
                    assert (derived.kind == clang.cindex.CursorKind.CLASS_DECL
                            or derived.kind == clang.cindex.CursorKind.STRUCT_DECL
                            or derived.kind == clang.cindex.CursorKind.CLASS_TEMPLATE)
                    
                    # Find the base classes for the derived class
                    base_classes: list[clang.cindex.Cursor] = []
                    for child in derived.get_children():
                        if child.kind != clang.cindex.CursorKind.CXX_BASE_SPECIFIER:
                            continue

                        class_or_struct_node = clang.cindex.conf.lib.clang_getCursorDefinition(child)
                        if class_or_struct_node is None:
                            continue

                        base_classes.append(class_or_struct_node)
                        if not clang.cindex.conf.lib.clang_isVirtualBase(child):
                            continue

                        # Search the immediate base classes for the function name
                        for other_child in class_or_struct_node.get_children():
                            if (other_child.displayname == node.displayname):
                                result.add(f"virtual?thunk?to?{namespace_str};")
                                return True

                    # Looks like it wasn't in any of our direct ancestors, so let's 
                    # try their ancestors too
                    for base in base_classes:
                        if search_class_hierarchy_for_virtual_thunk(base):
                            return True
                        
                    return False
                    
                search_class_hierarchy_for_virtual_thunk(node.semantic_parent)
                            

    # Traverse down the tree if we can
    if (node.kind == clang.cindex.CursorKind.TRANSLATION_UNIT
        or node.kind == clang.cindex.CursorKind.CLASS_DECL
        or node.kind == clang.cindex.CursorKind.STRUCT_DECL
        or node.kind == clang.cindex.CursorKind.NAMESPACE):
        for child in node.get_children():
            traverse_ast(child, filename, result)

    return result


def process_directory(directory: str, search_dirs: Optional[list[str]]) -> list[str]:
    current_set = set()

    files_dir = get_absolute_path_form_project_path(directory)
    _logger.debug(f"Processing external directory: {files_dir}")
    files = glob.glob(files_dir + '/**/*.h', recursive=True)
    search_variables = []
    for dir in search_dirs:
        search_variables.append(f"-I{get_absolute_path_form_project_path(dir)}")
    args = ['-std=c++20', '-x', 'c++-header', '-nostdlibinc'] + search_variables
    for file in files:
        idx = clang.cindex.Index.create()
        tu = idx.parse(
            file, 
            args=args,  
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
        library_data: LibraryInfo = library_to_data_map[library]
    except KeyError:
        _logger.error(f"The provided library has yet to be implmented: {library}")
        exit(1)

    _logger.debug(f"Generating symbols for {library}")

    search_dirs = library_data["search_headers"]
    # First, we process the external symbols
    external_symbols = []
    for header_dir in library_data["external_headers"]:
        external_symbols += process_directory(header_dir, search_dirs)
    output_symbols_to_file(
        external_symbols,
        library,
        args.symbol_version,
        library_data["external_output_file"])
    
    # Then, we process the internal symbols
    internal_symbols = []
    if "internal_headers" in library_data:
        for header_dir in library_data["internal_headers"]:
            internal_symbols += process_directory(header_dir, search_dirs)
        
        if "internal_output_file" in library_data:
            output_symbols_to_file(
                internal_symbols,
                library,
                args.symbol_version,
                library_data["internal_output_file"])
    exit(0)

    

if __name__ == "__main__":
    main()
