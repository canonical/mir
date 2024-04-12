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
from collections import OrderedDict
import bisect
import clang.cindex

_logger = logging.getLogger(__name__)

LibraryName = Literal[
    "miral", "mir_common", "mir_core", "miroil", "mir_platform",
    "mir_renderer", "mir_renderers", "mir_server_internal", "mir_test", "mir_wayland"]


class HeaderDirectory(TypedDict):
    path: str
    is_internal: bool


class LibraryInfo(TypedDict):
    header_directories: list[HeaderDirectory]
    map_file: str
    include_headers: Optional[list[str]]


class Symbol:
    name: str
    version: str
    is_c_symbol: bool
    
    def __init__(self, name: str, version: str, is_c_symbol: bool):
        self.name = name
        self.version = version
        self.is_c_symbol = is_c_symbol

    def is_internal(self) -> bool:
        return self.version.startswith("INTERNAL_")


# Directory paths are relative to the root of the Mir project
library_to_data_map: dict[LibraryName, LibraryInfo] = {
    "miral": {
        "header_directories": [
            {"path": "include/miral", "is_internal": False}
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
    "miroil": {
        "header_directories": [
            {"path": "include/miroil", "is_internal": False}
        ],
        "map_file": "src/miroil/symbols.map",
        "include_headers": [
            "include/platform",
            "include/gl",
            "include/renderers/gl"
            "include/server",
            "include/core",
            "include/client",
            "include/miral",
            "src/include/server",
        ]
    }
}


def get_absolute_path_from_project_path(project_path: str) -> Path:
    return Path(__file__).parent.parent.parent / project_path


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


def traverse_ast(node: clang.cindex.Cursor, filename: str, result: set[str], current_namespace: str = "") -> set[str]:
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
          and not node.is_pure_virtual_method()
          and not node.is_anonymous()):
        
        if current_namespace:
            namespace_str = f"{current_namespace}::{create_node_symbol_name(node)}"
        else:
            namespace_str = create_node_symbol_name(node)

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
    is_file = node.kind == clang.cindex.CursorKind.TRANSLATION_UNIT
    is_containing_node = (node.kind == clang.cindex.CursorKind.CLASS_DECL
        or node.kind == clang.cindex.CursorKind.STRUCT_DECL
        or node.kind == clang.cindex.CursorKind.NAMESPACE)
    if ((is_file and node.spelling == filename) or (is_containing_node and node.location.file.name == filename)):
        if node.kind != clang.cindex.CursorKind.TRANSLATION_UNIT:
            if not current_namespace:
                current_namespace = node.spelling
            else:
                current_namespace = f"{current_namespace}::{node.spelling}"

        for child in node.get_children():
            traverse_ast(child, filename, result, current_namespace)

    return result


def process_directory(directory: str, search_dirs: Optional[list[str]]) -> set[str]:
    result = set()

    files_dir = get_absolute_path_from_project_path(directory)
    _logger.debug(f"Processing directory: {files_dir}")
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


def read_symbols_from_file(file: Path, library_name: str) -> list[Symbol]:
    """
    Returns a list of symbols for each stanza in the symbols.map file.
    The returned list is ordered by version.
    """
    # WARNING: This is a very naive way to parse these files
    library_name = library_name.upper() + "_"
    retval: list[Symbol] = []
    version_str = None
    with open(file.as_posix()) as f:
        for line in f.readlines():
            if line.startswith(library_name):
                # This denotes that a new version
                version_str = line[len(library_name):].split()[0]
            elif line.startswith("    "):
                line = line.strip()
                assert line.endswith(';')
                retval.append(Symbol(line, version_str, False))
            elif line.startswith("  "):
                line = line.strip()
                if line.startswith("global:") or line.startswith("local:") or line == 'extern "C++" {' or line.startswith("}"):
                    continue
                
                # This is a c-symbol
                assert line.endswith(';')
                retval.append(Symbol(line, version_str, True))
    return retval


def get_added_symbols(previous_symbols: list[Symbol], new_symbols: set[str], is_internal: bool) -> list[str]:
    added_symbols: set[str] = new_symbols.copy()
    for symbol in previous_symbols:
        if (symbol.name in added_symbols) or (is_internal != symbol.is_internal and symbol.name in added_symbols):
            added_symbols.remove(symbol.name)
    added_symbols = list(added_symbols)
    added_symbols.sort()
    return added_symbols


def print_symbols_diff(previous_symbols: list[Symbol], new_symbols: set[str], is_internal: bool):
    """
    Prints the diff between the previous symbols and the new symbols.
    """
    added_symbols = get_added_symbols(previous_symbols, new_symbols, is_internal)
    
    print("")
    print("\033[1mNew Symbols 🟢🟢🟢\033[0m")
    for s in added_symbols:
        print(f"\033[92m    {s}\033[0m")

    # Deleted symbols
    deleted_symbols = set()
    for symbol in previous_symbols:
        if is_internal == symbol.is_internal and not symbol.name in new_symbols:
            deleted_symbols.add(symbol.name)
    deleted_symbols = deleted_symbols - new_symbols
    deleted_symbols = list(deleted_symbols)
    deleted_symbols.sort()

    print("")
    print("\033[1mDeleted Symbols 🔻🔻🔻\033[0m")
    for s in deleted_symbols:
        print(f"\033[91m    {s}\033[0m")


def main():
    parser = argparse.ArgumentParser(description="This tool parses the header files of a library in the Mir project "
                                        "and outputs a list of new and removed symbols either to stdout or a symbols.map file. "
                                        "With this list, a developer may update the corresponding symbols.map appropriately. "
                                        "The tool uses https://pypi.org/project/libclang/ to process "
                                        "the AST of the project's header files.")
    choices = list(get_args(LibraryName))
    parser.add_argument('--library', type=str,
                    help=f'library to generate symbols for ({", ".join(choices)})',
                    required=True,
                    choices=list(choices))
    parser.add_argument('--version', type=str,
                    help='Current version of the library',
                    required=True)
    parser.add_argument('--diff', action='store_true',
                    help='If true a diff should be output to the console')
    parser.add_argument('--output_symbols', action='store_true',
                        help='If true, the symbols.map file will be updated with the new new symbols')
    
    args = parser.parse_args()

    logging.basicConfig(level=logging.DEBUG)
    library = args.library
    version = args.version

    # Remove the patch version since we're not interested in it
    split_version = version.split('.')
    if len(split_version) == 3:
        version = f"{split_version[0]}.{split_version[1]}"

    _logger.info(f"Symbols map generation is beginning for library={library} with version={version}")

    try:
        library_data: LibraryInfo = library_to_data_map[library]
    except KeyError:
        _logger.error(f"The provided library has yet to be implmented: {library}")
        exit(1)

    if "include_headers" in library_data:
        search_dirs = library_data["include_headers"]
    else:
        search_dirs = []

    # Create a set that includes all of the available symbols
    external_symbols: set[str] = set()
    internal_symbols: set[str] = set()
    for directory in library_data["header_directories"]:
        path = directory["path"]
        symbols = process_directory(path, search_dirs)
        if directory["is_internal"]:
            internal_symbols = internal_symbols.union(symbols)
        else:
            external_symbols = external_symbols.union(symbols)
    
    previous_symbols = read_symbols_from_file(
        get_absolute_path_from_project_path(library_data["map_file"]), library)

    if args.diff:
        print_symbols_diff(previous_symbols, external_symbols, False)
        print_symbols_diff(previous_symbols, internal_symbols, True)
        print("")

    if args.output_symbols:
        _logger.info(f"Outputting the symbols file to: {get_absolute_path_from_project_path(library_data['map_file'])}")

        # We now have a list of new external and internal symbols. Our goal now is to add them to the correct stanzas
        new_external_symbols = get_added_symbols(previous_symbols, external_symbols, False)
        new_internal_symbols = get_added_symbols(previous_symbols, internal_symbols, True)

        next_version = f"{library.upper()}_{version}"
        next_internal_version = f"{library.upper()}_INTERNAL_{version}"
        data_to_output: OrderedDict[str, dict[str, list[str]]] = OrderedDict()

        # Remake the stanzas for the previous symbols
        for symbol in previous_symbols:
            version = f"{library.upper()}_{symbol.version}"
            if not version in data_to_output:
                data_to_output[version] = {
                    "c": [],
                    "c++": []
                }
            if symbol.is_c_symbol:
                bisect.insort(data_to_output[version]["c"], symbol.name)
            else:
                bisect.insort(data_to_output[version]["c++"], symbol.name)

        # Add the external symbols
        for symbol in new_external_symbols:
            if not next_version in data_to_output:
                data_to_output[next_version] = {
                    "c": [],
                    "c++": []
                }
            bisect.insort(data_to_output[next_version]["c++"], symbol)

        # Add the internal symbols
        for symbol in new_internal_symbols:
            if not next_internal_version in data_to_output:
                data_to_output[next_internal_version] = {
                    "c": [],
                    "c++": []
                }
            bisect.insort(data_to_output[next_internal_version]["c++"], symbol)

        # Finally, output them to the file
        with open(get_absolute_path_from_project_path(library_data["map_file"]), 'w+') as f:
            prev_internal_version_str = None
            prev_external_version_str = None
            for i, (version, symbols_dict) in enumerate(data_to_output.items()):
                # Only the first stanza should contain the local symbols
                if i == 0:
                    closing_line = "local: *;\n"
                else:
                    closing_line = ""

                # Add the correct previous version. This will depend on 
                is_internal = "_INTERNAL_" in version
                if is_internal:
                    if prev_internal_version_str is not None:
                        closing_line += "} " + prev_internal_version_str + ";"
                    else:
                        closing_line += "};"
                    prev_internal_version_str = version
                else:
                    if prev_external_version_str is not None:
                        closing_line += "} " + prev_external_version_str + ";"
                    else:
                        closing_line += "};"
                    prev_external_version_str = version

                if not symbols_dict["c"]:
                    c_symbols_str = ""
                else:
                    c_symbols_str = "\n  " + "\n  ".join(symbols_dict["c"]) + "\n"
                cpp_symbols_str = "\n    ".join(symbols_dict["c++"])
                output_str = f'''{version} {"{"}
global:{c_symbols_str}
  extern "C++" {"{"}
    {cpp_symbols_str}
  {'};'}
{closing_line}

'''

                f.write(output_str)
            

    exit(0)

    

if __name__ == "__main__":
    main()
