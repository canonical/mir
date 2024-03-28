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
import subprocess
from pathlib import Path
from typing import Literal, TypedDict
import clang.cindex

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
        "output_file": "src/miral/symbols1.map",
        "dependencies": [
            "include/miral/",
            "include/core/",
            "include/common/"
        ]
    }
}

def to_definition(namespace_stack: list[str], value: str):
    if (len(namespace_stack) == 0):
        return value
    return "::".join(namespace_stack) + "::" + value

def traverse(node: clang.cindex.Cursor, namespace_stack: list[str]):
    next_namespace_stack = namespace_stack.copy()
    if node.kind == clang.cindex.CursorKind.CLASS_DECL:
        next_namespace_stack.append(node.displayname)
    elif node.kind == clang.cindex.CursorKind.STRUCT_DECL:
        next_namespace_stack.append(node.displayname)
    elif node.kind == clang.cindex.CursorKind.NAMESPACE:
        next_namespace_stack.append(node.displayname)
    elif (node.kind == clang.cindex.CursorKind.FUNCTION_DECL 
          or node.kind == clang.cindex.CursorKind.CXX_METHOD):
        print(node.lexical_parent.displayname)
        print(to_definition(namespace_stack, node.displayname))

    for child in node.get_children():
        traverse(child, next_namespace_stack)

def main():
    logging.debug("Symbols map generation is beginning")
    s = '''
    #include "x.h" 
    namespace mir{}
    struct X{ void get() {} };
    int fac(int n) {
        return (n>1) ? n*fac(n-1) : 1;
    }
    '''

    # idx = clang.cindex.Index.create()
    # tu = idx.parse('tmp.cpp', args=['-std=c++20'],  
    #                 unsaved_files=[('tmp.cpp', s)],  options=0)
    # root = tu.cursor        
    # traverse(root, [])
    for library in library_to_header_map:
        directories: HeaderDirectories = library_to_header_map[library]
        include_flags: list[str] = []
        for include in directories["dependencies"]:
            include_path = Path.cwd().parent.joinpath(include).absolute().as_posix()
            include_flags.append("-I")
            include_flags.append(include_path)

        for external_directory in directories["external"]:
            path = Path.cwd().parent.joinpath(external_directory)
            files = glob.glob(path.absolute().as_posix() + '/**/*.h', recursive=True)
            for file in files:
                s = open(file).read()
                idx = clang.cindex.Index.create()
                tu = idx.parse('tmp.cpp', args=['-std=c++20'],  
                                unsaved_files=[('tmp.cpp', s)],  options=0)
                root = tu.cursor        
                traverse(root, [])
                return
                # cmd = [
                #     "clang++-19",
                #     "-extract-api",
                #     "-x",
                #     "c++-header",
                #     file,
                #     "-std=c++2b"
                # ] + include_flags
                # print(cmd)
                # subprocess.run(cmd) 

    

if __name__ == "__main__":
    main()
