#!/usr/bin/env python
# coding: utf-8

# Copyright © 2022 Canonical Ltd.
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

import sys
import os
import re
import subprocess

def run_get_output(args):
    return subprocess.run(
        args,
        capture_output=True,
        encoding='utf-8',
        check=True).stdout

def canonicalize_lib_name(name):
    return re.sub(r'[^A-Za-z0-9]', '', name.lower())

def split_name_and_types(sym):
    parts = re.split(r'([\(<])', sym)
    name = parts[0]
    types = ''.join(parts[1:])
    return name, types

def print_sym_set(s, text):
    print()
    print(str(len(s)) + ' ' + text + ':')
    for sym in s:
        print('  ' + str(sym))

class Symbol:
    def __init__(self, prefix, name, types, lib_id, version):
        self.prefix = prefix
        self.name = name
        self.types = types
        self.lib_id = lib_id
        self.version = version

    def __str__(self):
        return self.prefix + '"' + self.name + self.types + '@' + self.lib_id + '_' + self.version + '"'

def dict_contains(d, symbol):
    if not symbol.name in d:
        return False
    for candidate in d[symbol.name]:
        if (candidate.name == symbol.name and
            candidate.types == symbol.types and
            candidate.lib_id == symbol.lib_id and
            candidate.version == symbol.version
        ):
            return True
    return False

assert len(sys.argv) == 2, 'wrong number of args, just provide path to .so with symbols you want to scan'
lib_path = os.path.realpath(sys.argv[1])
assert os.path.isfile(lib_path), lib_path + ' is not a file'
groups = re.search(r'(lib[^/]*)\.so\.(\d+)', lib_path)
assert groups, 'could not guess debian symbols file name from ' + lib_path
lib_name = groups.group(1)
so_version = int(groups.group(2))
debian_symbols_path = os.path.join(
    os.path.dirname(os.path.dirname(__file__)),
    'debian',
    lib_name + str(so_version) + '.symbols')
assert os.path.isfile(debian_symbols_path), debian_symbols_path + ' is not a file'
raw_nm = run_get_output(['nm', '-D', lib_path])
lib_symbols = {}
for raw_line in raw_nm.splitlines():
    if '@' in raw_line:
        groups = re.search(
            r' (?P<type>\w) (?P<c_sym>[^\n@]*)@+(?P<lib>[\w]+)_(?P<version>[\d\.]+)',
            raw_line)
        assert groups, 'could not parse nm line: ' + raw_line
        c_sym = groups.group('c_sym')
        lib_id = groups.group('lib')
        version = groups.group('version')
        cpp_sym = run_get_output(['c++filt', c_sym]).strip()
        cpp_name, cpp_types = split_name_and_types(cpp_sym)
        if canonicalize_lib_name(lib_id) in canonicalize_lib_name(lib_name):
            lib_symbols.setdefault(cpp_name, [])
            lib_symbols[cpp_name].append(
                Symbol('(c++)', cpp_name, cpp_types, lib_id, version))
print('read ' + str(len(lib_symbols)) + ' symbols from ' + lib_path)
print('parsing ' + debian_symbols_path)
with open(debian_symbols_path, 'r') as f:
    debian_symbols_raw = f.read()
debian_symbols = {}
for raw_line in debian_symbols_raw.splitlines():
    cpp_groups = re.search(
        r'(?P<prefix>\(c\+\+.*\))"(?P<cpp_sym>[^@]*)@(?P<lib>\w+)_(?P<version>[\d\.]+)" (?P<outer_version>[\d\.]+)',
        raw_line)
    if raw_line.strip().startswith('#'):
        pass
    elif cpp_groups:
        prefix = cpp_groups.group('prefix')
        cpp_sym = cpp_groups.group('cpp_sym')
        cpp_name, cpp_types = split_name_and_types(cpp_sym)
        lib_id = cpp_groups.group('lib')
        version = cpp_groups.group('version')
        outer_version = cpp_groups.group('outer_version')
        if not outer_version.startswith(version):
            print(
                'WARNING: ' + cpp_sym +
                ' in ' + debian_symbols_path +
                ' has mismatched versions (inner version: ' +
                version + ', outer version: ' +
                outer_version + ')')
        debian_symbols.setdefault(cpp_name, [])
        debian_symbols[cpp_name].append(
            Symbol(prefix, cpp_name, cpp_types, lib_id, version))
    else:
        print('could not parse "' + raw_line + '" in ' + debian_symbols_path)
only_in_debian = set()
only_in_debian_32_bit = set()
in_both = set()
for syms in debian_symbols.values():
    for sym in syms:
        if dict_contains(lib_symbols, sym):
            in_both.add(sym)
        elif 'arch-bits=32' in sym.prefix:
            only_in_debian_32_bit.add(sym)
        else:
            only_in_debian.add(sym)
only_in_lib = set()
for syms in lib_symbols.values():
    for sym in syms:
        if not dict_contains(debian_symbols, sym):
            only_in_lib.add(sym)
print()
print(str(len(in_both)) + ' in both library and .symbols file.')
print_sym_set(only_in_debian_32_bit, '32-bit symbols only in .symbols file')
print_sym_set(only_in_debian, 'symbols in only the .symbols file')
print_sym_set(only_in_lib, 'symbols in only the library')
