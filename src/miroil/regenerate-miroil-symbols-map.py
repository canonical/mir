#! /usr/bin/python3
"""This script processes the XML generated by "make doc" and produces summary information
on symbols that libmiroil intends to make public.

To use: Go to your build folder and run "make regenerate-miroil-symbols-map" """

from xml.dom import minidom
from sys import argv

HELPTEXT = __doc__
DEBUG = False

def _get_text(node):
    substrings = []
    for node in node.childNodes:
        if node.nodeType == node.TEXT_NODE:
            substrings.append(node.data)
        elif node.nodeType == node.ELEMENT_NODE:
            substrings.append(_get_text(node))
    return ''.join(substrings)

def _get_text_for_element(parent, tagname):
    substrings = []

    for node in parent.getElementsByTagName(tagname):
        substrings.append(_get_text(node))

    return ''.join(substrings)

def _get_file_location(node):
    for node in node.childNodes:
        if node.nodeType == node.ELEMENT_NODE and node.tagName == 'location':
            return node.attributes['file'].value
    if DEBUG:
        print('no location in:', node)
    return None

def _has_element(node, tagname):
    for node in node.childNodes:
        if node.nodeType == node.ELEMENT_NODE and node.tagName in tagname:
            return True
    return False

def _print_attribs(node, attribs):
    for attrib in attribs:
        print(' ', attrib, '=', node.attributes[attrib].value)

def _concat_text_from_tags(parent, tagnames):
    substrings = []

    for tag in tagnames:
        substrings.append(_get_text_for_element(parent, tag))

    return ''.join(substrings)

def _print_location(node):
    print(' ', 'location', '=', _get_file_location(node))

def _get_attribs(node):
    kind = node.attributes['kind'].value
    static = node.attributes['static'].value
    prot = node.attributes['prot'].value
    return kind, static, prot

COMPONENT_MAP = {}
SYMBOLS = {'public' : set(), 'private' : set()}

def _report(publish, symbol):
    symbol = symbol.replace('~', '?')

    if publish:
        SYMBOLS['public'].add(symbol)
    else:
        SYMBOLS['private'].add(symbol)

    if not DEBUG:
        return

    if publish:
        print('  PUBLISH: {}'.format(symbol))
    else:
        print('NOPUBLISH: {}'.format(symbol))

OLD_STANZAS = '''MIROIL_2.0 {
global:'''

END_NEW_STANZA = '''
local: *;
};'''

def _print_report():
    print(OLD_STANZAS)
    new_symbols = False;
    for symbol in sorted(SYMBOLS['public']):
        formatted_symbol = '    {};'.format(symbol)
        if formatted_symbol not in OLD_STANZAS and 'miroil::' in formatted_symbol:
            if not new_symbols:
                new_symbols = True;
                print('  extern "C++" {')
            print(formatted_symbol)

    if new_symbols: print("  };")
    print(END_NEW_STANZA)

def _print_debug_info(node, attributes):
    if not DEBUG:
        return
    print()
    _print_attribs(node, attributes)
    _print_location(node)

def _parse_member_def(context_name, node, is_class):
    kind = node.attributes['kind'].value

    if (kind in ['enum', 'typedef']
        or _has_element(node, ['templateparamlist'])
        or kind in ['function'] and node.attributes['inline'].value == 'yes'):
        return

    name = _concat_text_from_tags(node, ['name'])

    if name in ['__attribute__']:
        if DEBUG:
            print('  ignoring doxygen mis-parsing:', _concat_text_from_tags(node, ['argsstring']))
        return

    if name.startswith('operator'):
        name = 'operator'

    if not context_name is None:
        symbol = context_name + '::' + name
    else:
        symbol = name

    is_function = kind == 'function'

    if is_function:
        _print_debug_info(node, ['kind', 'prot', 'static', 'virt'])
    else:
        _print_debug_info(node, ['kind', 'prot', 'static'])

    if DEBUG:
        print('  is_class:', is_class)

    publish = _should_publish(is_class, is_function, node)

    _report(publish, symbol + '*')

    if is_function and node.attributes['virt'].value == 'virtual':
        _report(publish, 'non-virtual?thunk?to?' + symbol + '*')


def _should_publish(is_class, is_function, node):
    (kind, static, prot) = _get_attribs(node)

    publish = True

    if publish:
        publish = kind != 'define'

    if publish and is_class:
        publish = is_function or static == 'yes'

    if publish and prot == 'private':
        if is_function:
            publish = node.attributes['virt'].value == 'virtual'
        else:
            publish = False

    if publish and _has_element(node, ['argsstring']):
        publish = not _get_text_for_element(node, 'argsstring').endswith('=0')

    return publish


def _parse_compound_defs(xmldoc):
    compounddefs = xmldoc.getElementsByTagName('compounddef')
    for node in compounddefs:
        kind = node.attributes['kind'].value

        if kind in ['page', 'file', 'example', 'union']:
            continue

        if kind in ['group']:
            for member in node.getElementsByTagName('memberdef'):
                _parse_member_def(None, member, False)
            continue

        if kind in ['namespace']:
            symbol = _concat_text_from_tags(node, ['compoundname'])
            for member in node.getElementsByTagName('memberdef'):
                _parse_member_def(symbol, member, False)
            continue

        filename = _get_file_location(node)

        if DEBUG:
            print('  from file:', filename)

        if ('/examples/' in filename or '/test/' in filename or '[generated]' in filename
            or '[STL]' in filename or _has_element(node, ['templateparamlist'])):
            continue

        symbol = _concat_text_from_tags(node, ['compoundname'])

        publish = True

        if publish:
            if kind in ['class', 'struct']:
                prot = node.attributes['prot'].value
                publish = prot != 'private'
                _print_debug_info(node, ['kind', 'prot'])
                _report(publish, 'vtable?for?' + symbol)
                _report(publish, 'typeinfo?for?' + symbol)

        if publish:
            for member in node.getElementsByTagName('memberdef'):
                _parse_member_def(symbol, member, kind in ['class', 'struct'])

if __name__ == "__main__":
    if len(argv) == 1 or '-h' in argv or '--help' in argv:
        print(HELPTEXT)
        exit()

    for arg in argv[1:]:
        try:
            if DEBUG:
                print('Processing:', arg)
            _parse_compound_defs(minidom.parse(arg))
        except Exception as error:
            print('Error:', arg, error)

    if DEBUG:
        print('Processing complete')

    _print_report()
