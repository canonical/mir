#! /usr/bin/python
from xml.dom import minidom
from sys import argv

debug = True

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

def report(component, publish, symbol):
    if publish: print '  PUBLISH in {}: {}'.format(component, symbol)
    else      : print 'NOPUBLISH in {}: {}'.format(component, symbol)
    
def printDebugInfo(node, attributes):
    if not debug: return
    print
    printAttribs(node, attributes)
    printLocation(node)

def parseMemberDef(context_name, node):
    library = mappedPhysicalComponent(getLocationFile(node))
    (kind, static, prot) = getAttribs(node)
    if kind in ['enum']: return
    name = concatTextFromTags(node, ['name'])
    if name in ['__attribute__']:
        if debug: print '  ignoring doxygen mis-parsing:', concatTextFromTags(node, ['argsstring'])
        return
    if not context_name == None: symbol = context_name + '::' + name
    else: symbol = name
    publish = '/include/' in getLocationFile(node)
    if publish: publish = prot != 'private'
    if publish: publish = kind == 'function' or static == 'yes'
    if publish: publish = kind != 'define'
    printDebugInfo(node, ['kind', 'prot', 'static'])
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

        if kind == 'group': 
            for member in node.getElementsByTagName('memberdef') : 
                parseMemberDef(None, member)
            continue
        
        file = getLocationFile(node)
        if '/examples/' in file or '/test/' in file or '[generated]' in file or '[STL]' in file: continue

        library = mappedPhysicalComponent(file)
        symbol = concatTextFromTags(node, ['compoundname'])

        if kind in ['class', 'struct']:
            prot =  node.attributes['prot'].value
            publish = '/include/' in getLocationFile(node)
            if publish: publish = prot != 'private'
            printDebugInfo(node, ['kind', 'prot'])
            report(library, publish, 'vtable?for?' + symbol)
            report(library, publish, 'typeinfo?for?' + symbol)

        for member in node.getElementsByTagName('memberdef') : 
            parseMemberDef(symbol, member)

if __name__ == "__main__":
    for arg in argv[1:]:
        try:
            if debug: print 'Processing:', arg
            xmldoc = minidom.parse(arg)
            parseCompoundDefs(xmldoc)
        except Exception as error:
            print 'Error:', arg, error
