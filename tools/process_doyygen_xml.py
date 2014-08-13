#! /usr/bin/python
from xml.dom import minidom
from sys import argv

debug = False

def getText(node):
    rc = []
    for node in node.childNodes:
        if node.nodeType == node.TEXT_NODE:
            rc.append(node.data)
        elif node.nodeType == node.ELEMENT_NODE:
            rc.append(getText(node))
    return ''.join(rc)

def getTextForElement(parent, tagname):
    rc = []
    nodes = parent.getElementsByTagName(tagname);
    for node in nodes : rc.append(getText(node))
    return ''.join(rc)

def getLocationFile(node):
    for node in node.childNodes:
        if node.nodeType == node.ELEMENT_NODE and node.tagName == 'location':
            return node.attributes['file'].value
    if debug: print 'no location in:', node
    return None
    
def hasElement(node, tagname):
    for node in node.childNodes:
        if node.nodeType == node.ELEMENT_NODE and node.tagName in tagname:
            return True
    return False

def printAttribs(node, attribs):
    for attrib in attribs : print ' ', attrib, '=', node.attributes[attrib].value

def concatTextFromTags(parent, tagnames):
    rc = []
    for tag in tagnames : rc.append(getTextForElement(parent, tag))
    return ''.join(rc)
    
def printLocation(node):
    print ' ', 'location', '=', getLocationFile(node)

def getAttribs(node):
    kind = node.attributes['kind'].value
    static = node.attributes['static'].value
    prot =  node.attributes['prot'].value
    return (kind, static, prot)

# Special cases for publishing anyway:
# In test_command_line_handling.cpp g++ sucessfully converts a virtual function call
# to a direct call to a private function: mir::options::DefaultConfiguration::the_options()
publish_special_cases = {'mir::options::DefaultConfiguration::the_options*'}

component_map = {}

def report(component, publish, symbol):
    symbol = symbol.replace('~', '?')

    if symbol in publish_special_cases: publish = True

    symbols = component_map.get(component, {'public' : set(), 'private' : set()})
    if publish: symbols['public'].add(symbol)
    else:       symbols['private'].add(symbol)
    component_map[component] = symbols
    if not debug: return
    if publish: print '  PUBLISH in {}: {}'.format(component, symbol)
    else      : print 'NOPUBLISH in {}: {}'.format(component, symbol)

def printReport():
    format = '{} {}: {};'
    for component, symbols in component_map.iteritems():
        print 'COMPONENT:', component
        for key in symbols.keys():
            for symbol in symbols[key]: print format.format(component, key, symbol)
        print

def printDebugInfo(node, attributes):
    if not debug: return
    print
    printAttribs(node, attributes)
    printLocation(node)

def parseMemberDef(context_name, node, is_class):
    library = mappedPhysicalComponent(getLocationFile(node))
    (kind, static, prot) = getAttribs(node)
    
    if kind in ['enum']: return
    if hasElement(node, ['templateparamlist']): return
    
    name = concatTextFromTags(node, ['name'])
    if name in ['__attribute__']:
        if debug: print '  ignoring doxygen mis-parsing:', concatTextFromTags(node, ['argsstring'])
        return

    if name.startswith('operator'): name = 'operator'
    if not context_name == None: symbol = context_name + '::' + name
    else: symbol = name

    publish = '/include/' in getLocationFile(node)
    if publish: publish = prot != 'private'
    if publish and is_class: publish = kind == 'function' or static == 'yes'
    if publish: publish = kind != 'define'
    printDebugInfo(node, ['kind', 'prot', 'static'])
    if debug: print '  is_class:', is_class
    report(library, publish, symbol+'*')

def findPhysicalComponent(location_file):
    path_elements = location_file.split('/')
    found = False
    for element in path_elements:
        if found: return element
        found = element in ['include', 'src']
    if debug: print 'no component in:', location_file
    return None
    
def mappedPhysicalComponent(location_file):
    location = findPhysicalComponent(location_file)
    if location == 'shared': location = 'common'
    return 'mir' + location

def parseCompoundDefs(xmldoc):
    compounddefs = xmldoc.getElementsByTagName('compounddef') 
    for node in compounddefs:
        kind = node.attributes['kind'].value

        if kind in ['page', 'file', 'example', 'union']: continue

        if kind in ['group']: 
            for member in node.getElementsByTagName('memberdef') : 
                parseMemberDef(None, member, False)
            continue

        if kind in ['namespace']: 
            symbol = concatTextFromTags(node, ['compoundname'])
            for member in node.getElementsByTagName('memberdef') : 
                parseMemberDef(symbol, member, False)
            continue
        
        file = getLocationFile(node)
        if debug: print '  from file:', file 
        if '/examples/' in file or '/test/' in file or '[generated]' in file or '[STL]' in file:
            continue

        if hasElement(node, ['templateparamlist']): continue

        library = mappedPhysicalComponent(file)
        symbol = concatTextFromTags(node, ['compoundname'])
        publish = '/include/' in getLocationFile(node)

        if publish: 
            if kind in ['class', 'struct']:
                prot =  node.attributes['prot'].value
                publish = prot != 'private'
                printDebugInfo(node, ['kind', 'prot'])
                report(library, publish, 'vtable?for?' + symbol)
                report(library, publish, 'typeinfo?for?' + symbol)

        if publish: 
            for member in node.getElementsByTagName('memberdef') : 
                parseMemberDef(symbol, member, kind in ['class', 'struct'])

if __name__ == "__main__":
    for arg in argv[1:]:
        try:
            if debug: print 'Processing:', arg
            xmldoc = minidom.parse(arg)
            parseCompoundDefs(xmldoc)
        except Exception as error:
            print 'Error:', arg, error

    printReport()