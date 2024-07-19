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
from typing import TypedDict, Optional
from collections import OrderedDict
import bisect
import clang.cindex
import os

_logger = logging.getLogger(__name__)

HIDDEN_SYMBOLS = {
    "miral::SessionLockListener::?SessionLockListener*;",
    "miral::WaylandExtensions::Context::?Context*;",
    "miral::WaylandExtensions::Context::Context*;",
    "vtable?for?miral::WaylandExtensions::Context;",
}

class HeaderDirectory(TypedDict):
    path: str
    is_internal: bool


class LibraryInfo(TypedDict):
    header_directories: list[HeaderDirectory]
    map_file: str

def get_version_from_library_version_str(version: str) -> str:
    """
    Given a string like MIR_SERVER_INTERNAL_5.0.0, returns the version
    string (e.g. 5.0.0)
    """
    return version.split("_")[-1]


def get_major_version_from_str(version: str) -> int:
    """
    Given a string like 5.0.0, returns the major version (e.g. 5).
    """
    return int(version.split('.')[0])


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


def has_vtable(node: clang.cindex.Cursor):
    # This method assumes that the node is a class/struct
    for child in node.get_children():
        if ((child.kind == clang.cindex.CursorKind.CXX_METHOD
            or child.kind == clang.cindex.CursorKind.DESTRUCTOR
            or child.kind == clang.cindex.CursorKind.CONSTRUCTOR
            or child.kind == clang.cindex.CursorKind.CONVERSION_FUNCTION)
            and child.is_virtual_method()):
            return True
        
    # Otherwise, look up the tree
    base_classes: list[clang.cindex.Cursor] = []
    for child in node.semantic_parent.get_children():
        if child.kind != clang.cindex.CursorKind.CXX_BASE_SPECIFIER:
            continue

        class_or_struct_node = clang.cindex.conf.lib.clang_getCursorDefinition(child)
        if class_or_struct_node is None:
            continue

        base_classes.append(class_or_struct_node)

    for base in base_classes:
        if has_vtable(base):
            return True

    return False


def has_virtual_base_class(node: clang.cindex.Cursor):
    # This method assumes that the node is a class/struct

    result = False
    for child in node.get_children():
        if child.kind != clang.cindex.CursorKind.CXX_BASE_SPECIFIER:
            continue

        if clang.cindex.conf.lib.clang_isVirtualBase(child):
            result = True
        else:
            class_or_struct_node = clang.cindex.conf.lib.clang_getCursorDefinition(child)
            if class_or_struct_node is None:
                continue

            result = has_virtual_base_class(class_or_struct_node)
        
        if result:
            break

    return result


def is_function_inline(node: clang.cindex.CursorKind):
    # This method assumes that the node is a FUNCTION_DECL.
    # There is no explicit way to check if a function is inlined
    # but seeing that if it is a FUNCTION_DECL and it has
    # a definition is good enough.
    return node.is_definition()


def get_namespace_str(node: clang.cindex.Cursor) -> list[str]:
    if node.kind == clang.cindex.CursorKind.TRANSLATION_UNIT:
        return []
    
    spelling = node.spelling
    if is_operator(node):
        spelling = "operator"
    elif node.kind == clang.cindex.CursorKind.DESTRUCTOR:
        spelling = f"?{node.spelling[1:]}"
    
    return get_namespace_str(node.semantic_parent) + [spelling]


def traverse_ast(node: clang.cindex.Cursor, filename: str, result: set[str]) -> set[str]:   
    # Ignore private and protected variables
    if (node.access_specifier == clang.cindex.AccessSpecifier.PRIVATE):
        return result
    
    # Check if we need to output a symbol
    if ((node.location.file is not None and node.location.file.name == filename)
        and ((node.kind == clang.cindex.CursorKind.FUNCTION_DECL and not is_function_inline(node))
          or node.kind == clang.cindex.CursorKind.CXX_METHOD
          or node.kind == clang.cindex.CursorKind.VAR_DECL
          or node.kind == clang.cindex.CursorKind.CONSTRUCTOR
          or node.kind == clang.cindex.CursorKind.DESTRUCTOR
          or node.kind == clang.cindex.CursorKind.CONVERSION_FUNCTION
          or should_generate_as_class_or_struct(node))
          and not node.is_pure_virtual_method()
          and not node.is_anonymous()):
        
        namespace_str = "::".join(get_namespace_str(node))
        _logger.debug(f"Emitting node {namespace_str} in file {node.location.file.name}")

        def add_symbol_str(s: str):
            if not s in HIDDEN_SYMBOLS:
                result.add(s)

        # Classes and structs have a specific output
        if (node.kind == clang.cindex.CursorKind.CLASS_DECL
            or node.kind == clang.cindex.CursorKind.STRUCT_DECL):
            if has_vtable(node):
                add_symbol_str(f"vtable?for?{namespace_str};")
            if has_virtual_base_class(node):
                add_symbol_str(f"VTT?for?{namespace_str};")
            add_symbol_str(f"typeinfo?for?{namespace_str};")
        else:
            def add_internal(s: str):
                add_symbol_str(f"{s}*;")
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
                            or derived.kind == clang.cindex.CursorKind.CLASS_TEMPLATE
                            or derived.kind == clang.cindex.CursorKind.TYPEDEF_DECL)
                    
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
    elif node.location.file is not None:
        _logger.debug(f"NOT emitting node {node.spelling} in file {node.location.file.name}")
                            

    # Traverse down the tree if we can
    is_file = node.kind == clang.cindex.CursorKind.TRANSLATION_UNIT
    is_containing_node = (node.kind == clang.cindex.CursorKind.CLASS_DECL
        or node.kind == clang.cindex.CursorKind.STRUCT_DECL
        or node.kind == clang.cindex.CursorKind.NAMESPACE)
    if is_file or is_containing_node:        
        if clang.cindex.conf.lib.clang_Location_isInSystemHeader(node.location):
            _logger.debug(f"Node is in a system header={node.location.file.name}")
            return result

        for child in node.get_children():
            traverse_ast(child, filename, result)
    else:
        _logger.debug(f"Nothing to process for node={node.spelling} in file={node.location.file.name}")

    return result


def process_directory(directory: Path, search_dirs: Optional[list[str]]) -> set[str]:
    result = set()

    files = directory.rglob('*.h')

    args = ['-std=c++23', '-x', 'c++-header']
    for dir in search_dirs:
        args.append(f"-I{dir}")

    for file in files:
        _logger.debug(f"Processing header file: {file.as_posix()}")
        file_args = args.copy()
        idx = clang.cindex.Index.create()
        tu = idx.parse(
            file.as_posix(), 
            args=file_args,  
            options=0)
        root = tu.cursor
        list(traverse_ast(root, file.as_posix(), result))

    return result


def read_symbols_from_file(f: argparse.FileType, library_name: str) -> list[Symbol]:
    """
    Returns a list of symbols for each stanza in the symbols.map file.
    The returned list is ordered by version.
    """
    # WARNING: This is a very naive way to parse these files
    library_name = library_name.upper() + "_"
    retval: list[Symbol] = []
    version_str = None

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


def report_symbols_diff(previous_symbols: list[Symbol], new_symbols: set[str], is_internal: bool):
    """
    Prints the diff between the previous symbols and the new symbols.
    """
    added_symbols = get_added_symbols(previous_symbols, new_symbols, is_internal)
    
    print("")
    print("  \033[1mNew Symbols 🟢🟢🟢\033[0m")
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
    print("  \033[1mDeleted Symbols 🔻🔻🔻\033[0m")
    for s in deleted_symbols:
        print(f"\033[91m    {s}\033[0m")

    return len(deleted_symbols) > 0 or len(added_symbols) > 0


def main():
    parser = argparse.ArgumentParser(description="This tool parses the header files of a provided in the Mir project "
                                        "and process a list of new internal and external symbols. "
                                        "By default, the script updates the corresponding symbols.map file automatically. "
                                        "To view this diff only, the user may provide the --diff option. "
                                        "The tool uses https://pypi.org/project/libclang/ to process "
                                        "the AST of the project's header files.\n\nIf the content of the outputted map "
                                        "file appears incorrect, trying using a later version of clang (e.g. clang 19). You can do this "
                                        "by specifying 'MIR_SYMBOLS_MAP_GENERATOR_CLANG_SO_PATH' along with 'MIR_SYMBOLS_MAP_GENERATOR_CLANG_LIBRARY_PATH'. "
                                        "Note that you most likely must set both of these values so that libLLVM-XY.so can be resolved alongside clang. "
                                        "As an example, I used this script (https://github.com/opencollab/llvm-jenkins.debian.net/blob/master/llvm.sh) "
                                        "to install clang 19 (sudo ./llvm.sh 19), and I set the following:"
                                        "\n\n    export MIR_SYMBOLS_MAP_GENERATOR_CLANG_SO_PATH=/usr/lib/llvm-19/lib/libclang.so.1"
                                        "\n    export MIR_SYMBOLS_MAP_GENERATOR_CLANG_LIBRARY_PATH=/usr/lib/llvm-19/lib"
                                        , formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('--library-name', type=str,
                    help='name of library',
                    dest="library_name",
                    required=True)
    parser.add_argument('--symbols-map-path', type=argparse.FileType('r+', encoding='latin-1'),
                    help='absolute path to a symbols map',
                    dest="symbols_map_path",
                    required=True)
    parser.add_argument('--external-headers-directory', type=Path,
                    help=f'absolute path to the directory containing the public-facing headers of this library',
                    dest="external_headers")
    parser.add_argument('--internal-headers-directory', type=Path,
                    help=f'absolute path to the directory containing private headers of this library',
                    dest="internal_headers")
    parser.add_argument('--version', type=str,
                    help='current version of the library',
                    required=True)
    parser.add_argument('--diff', action='store_true',
                    help='if true a diff should be output to the console')
    parser.add_argument('--include-dirs', type=str, help="colon separated list of directories to search for symbols",
                        required=True, dest='include_dirs')
    
    args = parser.parse_args()
    logging.basicConfig(level=logging.INFO)

    if args.diff:
        _logger.info("symbols_map_generator is running in 'diff' mode")
    else:
        _logger.info("symbols_map_generator is running in 'output symbols' mode")
    
    # Point libclang to a file on the system
    if 'MIR_SYMBOLS_MAP_GENERATOR_CLANG_SO_PATH' in os.environ:
        file_path = os.environ['MIR_SYMBOLS_MAP_GENERATOR_CLANG_SO_PATH']
        _logger.info(f"MIR_SYMBOLS_MAP_GENERATOR_CLANG_SO_PATH is {file_path}")
        clang.cindex.Config.set_library_file(file_path)
    else:
        _logger.error("Expected MIR_SYMBOLS_MAP_GENERATOR_CLANG_SO_PATH to be defined in the environment")
        exit(1)
    
    if 'MIR_SYMBOLS_MAP_GENERATOR_CLANG_LIBRARY_PATH' in os.environ:
        library_path = os.environ['MIR_SYMBOLS_MAP_GENERATOR_CLANG_LIBRARY_PATH']
        _logger.info(f"MIR_SYMBOLS_MAP_GENERATOR_CLANG_LIBRARY_PATH is {library_path}")
        clang.cindex.Config.set_library_path(library_path)
    else:
        _logger.error("Expected MIR_SYMBOLS_MAP_GENERATOR_CLANG_LIBRARY_PATH to be defined in the environment")
        exit(1)
    
    include_dirs = args.include_dirs.split(":")
    _logger.debug(f"Parsing with include directories: {include_dirs}")

    library = args.library_name
    version = args.version

    # Remove the patch version since we're not interested in it
    split_version = version.split('.')
    if len(split_version) == 3:
        version = f"{split_version[0]}.{split_version[1]}"

    _logger.info(f"Symbols map generation is beginning for library={library} with version={version}")

    # Create a set that includes all of the available symbols
    external_symbols: set[str] = set()
    if args.external_headers:
        _logger.info(f"Processing external headers directory: {args.external_headers}")
        external_symbols: set[str] = process_directory(
            args.external_headers,
            include_dirs
        )

    internal_symbols: set[str] = set()
    if args.internal_headers:
        _logger.info(f"Processing internal headers directory: {args.internal_headers}")
        internal_symbols: set[str] = process_directory(
            args.internal_headers,
            include_dirs
        )
    
    previous_symbols = read_symbols_from_file(args.symbols_map_path, library)

    has_changed_symbols = False
    if args.diff:
        print("External Symbols Diff:")
        has_changed_symbols = report_symbols_diff(previous_symbols, external_symbols, False)

        print("")
        print("Internal Symbols Diff:")
        has_changed_symbols = has_changed_symbols or report_symbols_diff(previous_symbols, internal_symbols, True)
        print("")
        if has_changed_symbols:
            exit(1)
    else:
        _logger.info(f"Outputting the symbols file to: {args.symbols_map_path.name}")

        # We now have a list of new external and internal symbols. Our goal now is to add them to the correct stanzas
        new_external_symbols = get_added_symbols(previous_symbols, external_symbols, False)
        new_internal_symbols = get_added_symbols(previous_symbols, internal_symbols, True)

        next_major = get_major_version_from_str(version)
        next_version = f"{library.upper()}_{version}"
        next_internal_version = f"{library.upper()}_INTERNAL_{version}"
        data_to_output: OrderedDict[str, dict[str, list[str]]] = OrderedDict()

        # Remake the stanzas for the previous symbols
        for symbol in previous_symbols:
            major = get_major_version_from_str(get_version_from_library_version_str(symbol.version))

            # If we are going up by a major version, then we should add
            # all existing symbols to the new stanza
            if major == next_major:
                symbol_version = f"{library.upper()}_{symbol.version}"
            else:
                symbol_version = f"{library.upper()}_{version}"
            
            if not symbol_version in data_to_output:
                data_to_output[symbol_version] = {
                    "c": [],
                    "c++": []
                }
            if symbol.is_c_symbol:
                bisect.insort(data_to_output[symbol_version]["c"], symbol.name)
            else:
                bisect.insort(data_to_output[symbol_version]["c++"], symbol.name)

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
        f = args.symbols_map_path
        f.seek(0)
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
    
        f.truncate()

if __name__ == "__main__":
    main()
