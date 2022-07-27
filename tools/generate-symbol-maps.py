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
symbol_re = re.compile(r'^[^\w\s:]$')
identifier_re = re.compile(r'^[\w:]+$')

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

def is_symbol(node):
    return isinstance(node, str) and symbol_re.match(node)

def is_identifier(node):
    return isinstance(node, str) and identifier_re.match(node)

def with_types_fixed(nodes):
    result = []
    prev = None
    for node in nodes:
        if node == 'std::string':
            result += parse_type(
                'std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>'
            ).children
        elif prev == 'std::vector':
            assert isinstance(node, Node) and node.children[0] == '<' and node.children[-1] == '>'
            vec_type = Node(node.children[1:-1]).emit()
            result.append(parse_type('<' + vec_type + ', std::allocator<' + vec_type + '>>'))
        elif prev == 'std::unique_ptr':
            assert isinstance(node, Node) and node.children[0] == '<' and node.children[-1] == '>'
            ptr_type = Node(node.children[1:-1]).emit()
            result.append(parse_type('<' + ptr_type + ', std::default_delete<' + ptr_type + '>>'))
        elif node == 'uint8_t':
            result += ['unsigned', 'char']
        elif isinstance(node, str) and node.startswith('::'):
            result.append(node[2:])
        else:
            result.append(node)
        prev = node
    return result

def without_arg_names(nodes):
    # Strip argument names from std::function type
    result = []
    has_type = False
    has_const = False
    for child in nodes:
        if child == ',':
            has_type = False
            has_const = False
            result.append(child)
        elif has_const and (child == '*' or child == '&'):
            result.append('const')
            result.append(child)
            has_const = False
        elif not is_identifier(child):
            result.append(child)
        elif child in ['long', 'unsigned', 'short', 'char', 'int', 'double', 'float']:
            has_type = True
            result.append(child)
        elif child == 'const':
            has_const = True
        elif not has_type:
            has_type = True
            result.append(child)
    return result

class Node:
    # children is a list containing AstNodes, symbol strings and identifier strings (see symbol_re and identifier_re)
    def __init__(self, children):
        assert children, 'node created with no children'
        for child in children:
            assert isinstance(child, Node) or is_symbol(child) or is_identifier(child), (
                'invalid AST node ' + repr(child)
            )
        self.children = children

    def fix(self):
        self.children = [child for child in self.children if child != 'struct']
        self.children = with_types_fixed(self.children)
        self.children = without_arg_names(self.children)
        for child in self.children:
            if isinstance(child, Node):
                child.fix()

    def emit(self):
        result = ''
        for i in range(len(self.children)):
            node = self.children[i]
            if i:
                prev = self.children[i - 1]
                if is_identifier(prev) and is_identifier(node):
                    result += ' '
                elif isinstance(prev, Node) and prev.children[-1] == '>' and node == '>':
                    result += ' '
                elif isinstance(prev, Node) and prev.children[-1] == ')' and is_identifier(node):
                    result += ' '
                elif node == 'const':
                    result += ' '
                elif isinstance(node, Node) and node.children[0] == '(':
                    result += ' '
            if isinstance(node, Node):
                result += node.emit()
            else:
                result += node
            if node == ',':
                result += ' '
        return result

def parse_type(text):
    nodes = ['']
    opening = set(['<', '('])
    closing = set(['>', ')'])
    brace_level = 0
    pending = ''
    first_char = True
    for c in text:
        if c in opening and not first_char:
            brace_level += 1
            pending += c
        elif brace_level:
            pending += c
            if c in closing:
                brace_level -= 1
                if brace_level == 0:
                    nodes += [parse_type(pending), '']
                    pending = ''
        elif is_symbol(c):
            nodes += [c, '']
        elif is_identifier(c):
            nodes[-1] += c
        else:
            nodes.append('')
        first_char = False
    return Node([node for node in nodes if node])

def format_type(text):
    ast = parse_type(text)
    ast.fix()
    return ast.emit()

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
    if result == 'auto':
        args_str = node.getElementsByTagName('argsstring')[0].firstChild.data
        if '->' in args_str:
            result = args_str.split('->', 1)[1].strip()
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
    type_str = resolve_type(symbols, node)
    abi = ''
    if 'std::string' in type_str:
        abi = '[abi:cxx11]'
    sym = full_name + abi + '(' + ', '.join(args) + ')' + (' const' if const else '')
    lib, version_major, version_minor = export_info
    symbols.add_symbol(lib, sym, version_major, version_minor)

def handle_node(symbols, node, export_info):
    kind = node.attributes['kind'].value
    id = node.attributes['id'].value
    try:
        if kind == 'class' or kind == 'struct':
            handle_class(symbols, node, export_info)
        elif kind == 'function':
            handle_function(symbols, node, export_info)
        elif kind == 'variable':
            handle_variable(symbols, node, export_info)
    except:
        print('error handling ' + id)
        raise

def handle_class(symbols, node, export_info):
    has_virtual = False
    for member in node.getElementsByTagName('memberdef'):
        if member.attributes['kind'].value == 'function' and member.attributes['prot'].value != 'private':
            if member.attributes['virt'].value == 'virtual' or member.attributes['virt'].value == 'pure-virtual':
                has_virtual = True
            # Ignore functions that are explicitly exported
            if not get_exported_info(member):
                handle_node(symbols, member, export_info)
    if has_virtual:
        class_name = node.getElementsByTagName('compoundname')[0].firstChild.data
        lib, version_major, version_minor = export_info
        symbols.add_symbol(lib, 'vtable for ' + class_name, version_major, version_minor)
        symbols.add_symbol(lib, 'typeinfo for ' + class_name, version_major, version_minor)

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
            for sym in sorted(list(symbols), key=str.lower):
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
        self.libs[lib].setdefault((version_major, version_minor), set())
        self.libs[lib][(version_major, version_minor)].add(sym)

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
