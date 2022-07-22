#! /usr/bin/python3
'''
This script parses the doxygen XML files and generates symbols.map files for our libraries

USAGE: python3 generate-symbols-maps.py <build/doc/xml/>
'''

import sys
import os
import re
from xml.dom import minidom

help_text = __doc__

exported_info_re = re.compile(r'since (\w+) (\d+)\.(\d+)', flags=re.I)

def get_exported_info(parent):
    for node in parent.getElementsByTagName('simplesect'):
        if node.attributes['kind'].value == 'remark':
            text = node.firstChild.firstChild.data
            result = exported_info_re.search(text)
            if result:
                lib = result.group(1)
                version_major = result.group(2)
                version_minor = result.group(3)
                return lib, version_major, version_minor
    return None

def drop_function_arg_names(text):
    parts = ['']
    opening = set(['<', '('])
    closing = set(['>', ')'])
    keywords = set(['const', 'struct'])
    i = 0
    has_name = False
    while i < len(text):
        # handle nested expressions
        if text[i] in opening:
            level = 1
            start = i
            while level and i < len(text) - 1:
                i += 1
                if text[i] in opening:
                    level += 1
                if text[i] in closing:
                    level -= 1
            inner = drop_function_arg_names(text[start + 1:i])
            if parts and parts[-1] == 'std::vector':
                inner = inner + ',std::allocator<' + inner + ' >'
            parts += [text[start] + inner + text[i], '']
        elif re.match(r'[\w:]', text[i]):
            parts[-1] += text[i]
            has_name = True
        else:
            parts += [text[i], '']
        i += 1
    result = ''
    # Only allow one arbitrary name unless separated by a comma
    has_user_id = False
    for part in parts:
        if re.match(r'[\w:]+', part) and part not in keywords:
            if has_user_id:
                # This is a function arg name, drop it
                pass
            else:
                result += part
                has_user_id = True
        elif part == 'long':
            has_user_id = True
        else:
            if part == ',':
                has_user_id = False
            result += part
    return result

def format_type(text):
    # Remove all unimportant whitespace
    special_chars = r'([\*&<>\(\),])'
    text = re.sub(r'\s+' + special_chars, '\\1', text)
    text = re.sub(special_chars + r'\s+', '\\1', text)
    text = re.sub(r'\s\s+', ' ', text)
    text = drop_function_arg_names(text)
    text = re.sub(r'([>,])', '\\1 ', text)
    text = re.sub(r'(\()', ' \\1', text)
    text = re.sub(r'\s*\)', ')', text)
    text = re.sub(r'struct\s*', '', text)
    text = re.sub(r'std::string', 'std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >', text)
    text = text.strip()
    return text

def resolve_type(symbols, node):
    result = ''
    id = None
    if 'id' in node.attributes:
        id = node.attributes['id'].value
    type_node = node.getElementsByTagName('type')[0]
    for child in type_node.childNodes:
        if child.nodeType == child.TEXT_NODE:
            result += child.data
        elif child.nodeType == child.ELEMENT_NODE:
            refid = child.attributes['refid'].value
            if refid == id:
                # Prevent recurive types when structs are typedefed to themselves
                result += child.childNodes[0].data
            else:
                result += symbols.get_type(refid)
    if node.getElementsByTagName('array'):
        result += '*'
    return result

def handle_variable(symbols, node, export_info):
    definition = node.getElementsByTagName('definition')[0].firstChild.data
    sym = re.search(r'[\w:]+$', definition).group(0)
    lib, version_major, version_minor = export_info
    symbols.add_symbol(lib, sym, version_major, version_minor)

def handle_function(symbols, node, export_info):
    definition = node.getElementsByTagName('definition')[0].firstChild.data
    name = node.getElementsByTagName('name')[0].firstChild.data
    assert definition.endswith(name), repr(definition) + ' does not end with ' + repr(name)
    def_without_name = definition[:-len(name)]
    namespace = re.search(r'[\w:]*$', def_without_name).group(0)
    full_name = namespace + name
    if re.search(r'\boperator ', full_name):
        parts = full_name.split('operator ', 1)
        parts[1] = format_type(parts[1])
        full_name = 'operator '.join(parts)
    const = node.attributes['const'].value == 'yes'
    args = []
    for param in node.getElementsByTagName('param'):
        args.append(format_type(resolve_type(symbols, param)))
    sym = full_name + '(' + ', '.join(args) + ')' + (' const' if const else '')
    lib, version_major, version_minor = export_info
    symbols.add_symbol(lib, sym, version_major, version_minor)

def handle_node(symbols, node, export_info):
    kind = node.attributes['kind'].value
    id = node.attributes['id'].value
    try:
        if kind == 'class':
            handle_class(symbols, node, export_info)
        elif kind == 'function':
            handle_function(symbols, node, export_info)
        elif kind == 'variable':
            handle_variable(symbols, node, export_info)
    except:
        print('error handling ' + id)
        raise

def handle_class(symbols, node, export_info):
    for member in node.getElementsByTagName('memberdef'):
        if member.attributes['kind'].value == 'function' and member.attributes['prot'].value != 'private':
            # Ignore functions that are explicitly exported
            if not get_exported_info(member):
                handle_node(symbols, member, export_info)

def handle_symbols(symbols, toplevel):
    for node in toplevel.getElementsByTagName('simplesect'):
        if node.attributes['kind'].value == 'remark':
            export_info = get_exported_info(node.parentNode)
            if export_info:
                while node.tagName != 'detaileddescription':
                    node = node.parentNode
                handle_node(symbols, node.parentNode, export_info)


def handle_typedef_type(symbols, node):
    id = node.attributes['id'].value
    def get_type_name():
        return resolve_type(symbols, node)
    symbols.add_type(id, get_type_name)

def handle_enum_type(symbols, node):
    name = node.getElementsByTagName('name')[0].firstChild.data
    id = node.attributes['id'].value
    symbols.add_type(id, lambda: name)

def handle_class_type(symbols, node):
    type_name = node.getElementsByTagName('compoundname')[0].firstChild.data
    id = node.attributes['id'].value
    def get_type_name():
        return type_name
    symbols.add_type(id, get_type_name)

def handle_types(symbols, toplevel):
    for node in toplevel.getElementsByTagName('memberdef'):
        kind = node.attributes['kind'].value
        if kind == 'typedef':
            handle_typedef_type(symbols, node)
        if kind == 'enum':
            handle_enum_type(symbols, node)
    for node in toplevel.getElementsByTagName('compounddef'):
        kind = node.attributes['kind'].value
        if kind == 'class' or kind == 'struct':
            handle_class_type(symbols, node)

def parse_all_xml(path):
    files = []
    items = os.listdir(path)
    print('parsing ' + str(len(items)) + ' XML files...')
    for item in items:
        if item.endswith('.xml'):
            filename = os.path.join(path, item)
            files.append((filename, minidom.parse(filename)))
    return files

def write_symbols_map_file(path, name, lib):
    with open(path, 'w') as f:
        prev_stanza = None
        for (major, minor), symbols in sorted(list(lib.items())):
            stanza = name + '_' + str(major) + '.' + str(minor)
            print('writing ' + stanza + '...')
            f.write(stanza + ' {\n')
            f.write('global:\n')
            f.write('  extern "C++" {\n')
            for sym in symbols:
                f.write('    "' + sym + '";\n')
            f.write('  };\n')
            if prev_stanza is None:
                f.write('local: *;\n')
                f.write('};\n')
            else:
                f.write('} ' + prev_stanza + ';\n')
            f.write('\n')
            prev_stanza = stanza

def write_symbols_map_files(symbols):
    mir_root = os.path.dirname(os.path.dirname(__file__))
    for name, lib in sorted(list(symbols.libs.items())):
        if name == 'MirAL':
            path = os.path.join(mir_root, 'src', 'miral', 'symbols.map')
            print('writing ' + path + '...')
            write_symbols_map_file(path, 'MIRAL', lib)
        else:
            assert False, 'Unknown lib name ' + repr(name)

class Symbols:
    def __init__(self):
        self.libs = {}
        self.types = {}
        # Used to prevent recursive type lookup
        self.currently_resolving = set()

    def add_symbol(self, lib, sym, version_major, version_minor):
        self.libs.setdefault(lib, {})
        self.libs[lib].setdefault((version_major, version_minor), [])
        self.libs[lib][(version_major, version_minor)].append(sym)

    def add_type(self, id, resolver):
        self.types[id] = resolver

    def get_type(self, id):
        assert id not in self.currently_resolving, 'type ' + id + ' is recursive'
        self.currently_resolving.add(id)
        result = self.types[id]()
        self.currently_resolving.remove(id)
        return result

    def pretty_str(self):
        result = ''
        for lib in sorted(list(self.libs.keys())):
            result += lib + ':\n'
            for major, minor in sorted(list(self.libs[lib].keys())):
                result += '  ' + str(major) + '.' + str(minor) + ':\n'
                for sym in sorted(list(self.libs[lib][(major, minor)])):
                    result += '    ' + sym + '\n'
                result += '\n'
        return result

if __name__ == "__main__":
    if '-h' in sys.argv or '--help' in sys.argv:
        print(help_text)
        exit()

    assert len(sys.argv) == 2, 'requires 1 arg'
    assert os.path.isdir(sys.argv[1]), sys.argv[1] + ' is not a directory'

    symbols = Symbols()
    print('looking for XML files...')
    xmls = parse_all_xml(sys.argv[1])
    print('indexing types...')
    for filename, parsed in xmls:
        try:
            handle_types(symbols, parsed)
        except:
            print('error in ' + filename)
            raise
    print('looking for symbols...')
    for filename, parsed in xmls:
        try:
            handle_symbols(symbols, parsed)
        except:
            print('error in ' + filename)
            raise
    print('writing symbols.map files...')
    write_symbols_map_files(symbols)
    print('done')
