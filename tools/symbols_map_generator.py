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
import argparse
from pathlib import Path
from typing import Literal, TypedDict, get_args, Optional
import clang.cindex

_logger = logging.getLogger(__name__)

LibraryName = Literal[
    "miral", "mir_common", "mir_core", "miroil", "mir_platform",
    "mir_renderer", "mir_renderers", "mir_server", "mir_test", "mir_wayland"]


class LibraryInfo(TypedDict):
    headers_dir: list[str]
    map_file: str
    include_headers: Optional[list[str]]


# Directory paths are relative to the root of the Mir project
library_to_data_map: dict[LibraryName, LibraryInfo] = {
    "miral": {
        "headers_dir": [
            "include/miral"
        ],
        "map_file": "src/miral/symbols.map",
        "include_headers": [
            "include/common",
            "include/core",
            "include/miral",
            "include/renderer",
            "include/server",
            "include/wayland"
        ]
    },
    "mir_server": {
        "headers_dir": [
            "include/server/mir",
            "src/include/server/mir"
        ],
        "map_file": "src/server/symbols.map",
        "include_headers": [
            "src/include/server"
        ]
    }
}


def get_absolute_path_from_project_path(project_path: str) -> Path:
    return Path(__file__).parent.parent / project_path


def should_generate_as_class_or_struct(node: clang.cindex.Cursor):
    return ((node.kind == clang.cindex.CursorKind.CLASS_DECL
          or node.kind == clang.cindex.CursorKind.STRUCT_DECL)
          and node.is_definition())


def is_operator(node: clang.cindex.Cursor) -> bool:
    if node.spelling.startswith("operator"):
        remainder = node.spelling[len("operator"):]
        if any(not c.isalnum() for c in remainder):
            return True
        
    return False


def create_node_symbol_name(node: clang.cindex.Cursor) -> str:
    if is_operator(node):
        return "operator"
    elif node.kind == clang.cindex.CursorKind.DESTRUCTOR:
        return f"?{node.spelling[1:]}"
    else:
        return node.spelling


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
          or node.kind == clang.cindex.CursorKind.CONVERSION_FUNCTION
          or should_generate_as_class_or_struct(node))
          and node.location.file.name == filename
          and not node.is_pure_virtual_method()):
        parent = node.semantic_parent
        namespace_str = create_node_symbol_name(node)

        # Walk up the tree to build the namespace
        while parent is not None:
            if parent.kind == clang.cindex.CursorKind.TRANSLATION_UNIT:
                break

            # TODO: FIX ME
            if parent.spelling == "std":
                break

            namespace_str = f"{parent.spelling}::{namespace_str}"
            parent = parent.semantic_parent

        # Classes and structs have a specific output
        if (node.kind == clang.cindex.CursorKind.CLASS_DECL
            or node.kind == clang.cindex.CursorKind.STRUCT_DECL):
            result.add(f"vtable?for?{namespace_str};")
            result.add(f"typeinfo?for?{namespace_str};")
        else:
            def add_internal(s: str):
                result.add(f"{s}*;")
            add_internal(namespace_str)
            
            # Check if we're marked virtual
            is_virtual = False
            if ((node.kind == clang.cindex.CursorKind.CXX_METHOD
                or node.kind == clang.cindex.CursorKind.DESTRUCTOR
                or node.kind == clang.cindex.CursorKind.CONSTRUCTOR
                or node.kind == clang.cindex.CursorKind.CONVERSION_FUNCTION)
                and node.is_virtual_method()):
                add_internal(f"non-virtual?thunk?to?{namespace_str}")
                is_virtual = True
            else:
                # Check if we're marked override
                for  child in node.get_children():
                    if child.kind == clang.cindex.CursorKind.CXX_OVERRIDE_ATTR:
                        add_internal(f"non-virtual?thunk?to?{namespace_str}")
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
                                add_internal(f"virtual?thunk?to?{namespace_str}")
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


def process_directory(directory: str, search_dirs: Optional[list[str]]) -> set[str]:
    result = set()

    files_dir = get_absolute_path_from_project_path(directory)
    _logger.debug(f"Processing external directory: {files_dir}")
    files = files_dir.rglob('*.h')
    search_variables = []
    for dir in search_dirs:
        search_variables.append(f"-I{get_absolute_path_from_project_path(dir).as_posix()}")
    args = ['-fsyntax-only', '-std=c++23', '-x', 'c++-header'] + search_variables
    for file in files:
        idx = clang.cindex.Index.create()
        tu = idx.parse(
            file.as_posix(), 
            args=args,  
            options=0)
        root = tu.cursor
        list(traverse_ast(root, file.as_posix(), result))

    return result 
    

def read_symbols_from_file(file: Path) -> set[str]:
    # WARNING: This is a very naive way to parse these files. We are assuming
    #  that any line starting with 4 spaces is a symbol.
    symbols: set[str] = set()
    with open(file.as_posix()) as f:
        for line in f.readlines():
            if line.startswith("    "):
                line = line.strip()
                assert line.endswith(';')
                symbols.add(line)
    return symbols


def main():
    parser = argparse.ArgumentParser(description="This tool parses the header files of a library in the Mir project "
                                        "and outputs a list of new and removed symbols to stdout. "
                                        "With this list, a developer may update the corresponding symbols.map appropriately. "
                                        "The tool uses https://pypi.org/project/libclang/ to process "
                                        "the AST of the project's header files.")
    choices = list(get_args(LibraryName))
    parser.add_argument('--library', type=str,
                    help=f'library to generate symbols for ({", ".join(choices)})',
                    required=True,
                    choices=list(choices))
    
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

    if "include_headers" in library_data:
        search_dirs = library_data["include_headers"]
    else:
        search_dirs = []

    # Create a set that includes all of the available headers
    new_symbols: set[str] = set()
    for header_dir in library_data["headers_dir"]:
        new_symbols = new_symbols.union(process_directory(header_dir, search_dirs))
    
    previous_symbols = read_symbols_from_file(
        get_absolute_path_from_project_path(library_data["map_file"]))

    # New symbols
    new_symbol_diff = list(new_symbols - previous_symbols)
    new_symbol_diff.sort()

    print("")
    print("\033[1mNew Symbols 🟢🟢🟢\033[0m")
    for s in new_symbol_diff:
        print(f"\033[92m    {s}\033[0m")

    # Deleted symbols
    deleted_symbols = list(previous_symbols - new_symbols)
    deleted_symbols.sort()
    print("")
    print("\033[1mDeleted Symbols 🔻🔻🔻\033[0m")
    for s in deleted_symbols:
        print(f"\033[91m    {s}\033[0m")

    exit(0)

    

if __name__ == "__main__":
    main()
