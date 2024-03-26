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

def main():
    logging.debug("Symbols map generation is beginning")
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
                cmd = [
                    "clang-18",
                    "-extract-api",
                    "-x",
                    "c++-header",
                    file,
                    "-std=c++2b"
                ] + include_flags
                print(cmd)
                subprocess.run(cmd) 

    

if __name__ == "__main__":
    main()
