#!/usr/bin/env python

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

# TODO:
#  1. Iterate the directories
#  2. For each directory, iterate each file

import logging
import glob
import os
from pathlib import Path
from typing import Literal, TypedDict
import clang.cindex

_logger = logging.getLogger(__name__)

LibraryName = Literal[
    "miral", "common", "core", "miroil", "platform",
    "renderer", "renderers", "server", "test", "wayland"]

class HeaderDirectories(TypedDict):
    external: list[str]
    internal: list[str]
    output_file: str
    dependencies: list[str]


# Directory paths are relative to the root of the Mir project
library_to_header_map: dict[LibraryName, HeaderDirectories] = {
    "miral": {
        "external": [
            "include/miral"
        ],
        "internal": [
            "src/include/miral"
        ],
        "output_file": "src/miral/symbols1.map"
    }
}

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
    for op in operators:
        if spelling == f"operator{op}":
            return True
        
    return False


def create_symbol_name(node: clang.cindex.Cursor) -> str:
    if node.kind == clang.cindex.CursorKind.DESTRUCTOR:
        return f"?{node.spelling[1:]}*"

    if ((node.kind == clang.cindex.CursorKind.FUNCTION_DECL 
        or node.kind == clang.cindex.CursorKind.CXX_METHOD)
        and is_operator_overload(node.spelling)):
        return "operator*"

    return f"{node.spelling}*"


def traverse(node: clang.cindex.Cursor, filename: str) -> set[str]:
    result: set[str] = set()
    if (node.access_specifier == clang.cindex.AccessSpecifier.PRIVATE
        or node.access_specifier == clang.cindex.AccessSpecifier.PROTECTED):
        pass
    if (node.kind == clang.cindex.CursorKind.FUNCTION_DECL 
          or node.kind == clang.cindex.CursorKind.CXX_METHOD
          or node.kind == clang.cindex.CursorKind.VAR_DECL
          or node.kind == clang.cindex.CursorKind.TYPE_ALIAS_DECL
          or node.kind == clang.cindex.CursorKind.CONSTRUCTOR
          or node.kind == clang.cindex.CursorKind.DESTRUCTOR):
        if (node.location.file.name == filename):
            parent = node.lexical_parent
            namespace_str = create_symbol_name(node)
            while parent is not None:
                if parent.kind == clang.cindex.CursorKind.TRANSLATION_UNIT:
                    break

                # TODO: Hack! Why is 'std' in the namespace...?
                if parent.kind == clang.cindex.CursorKind.NAMESPACE and parent.displayname == "std":
                    break

                namespace_str = f"{parent.displayname}::{namespace_str}"
                parent = parent.lexical_parent

            result.add(namespace_str)

    if (node.kind == clang.cindex.CursorKind.TRANSLATION_UNIT
        or node.kind == clang.cindex.CursorKind.CLASS_DECL
        or node.kind == clang.cindex.CursorKind.STRUCT_DECL
        or node.kind == clang.cindex.CursorKind.NAMESPACE):
        for child in node.get_children():
            result |= traverse(child, filename)

    return result

def process_directory(directory: str) -> list[str]:
    result: list[str] = []
    script_path = os.path.dirname(os.path.realpath(__file__))
    path = Path(script_path).parent.joinpath(directory)
    _logger.debug(f"Processing external directory: {path}")
    files = glob.glob(path.absolute().as_posix() + '/**/*.h', recursive=True)
    for file in files:
        idx = clang.cindex.Index.create()
        tu = idx.parse(
            file, 
            args=['-std=c++20', '-x', 'c++-header'],  
            options=0)
        root = tu.cursor
        file_result = list(traverse(root, file))
        result = result + file_result

        if _logger.getEffectiveLevel() == logging.DEBUG:
            for value in file_result:
                _logger.debug(f"{file}: {value}")

    return result

def main():
    logging.basicConfig(level=logging.DEBUG)
    _logger.debug("Symbols map generation is beginning")

    for library in library_to_header_map:
        _logger.debug(f"Generating symbols for {library}")
        directories: HeaderDirectories = library_to_header_map[library]

        for external_directory in directories["external"]:
            process_directory(external_directory)
            return

    

if __name__ == "__main__":
    main()
