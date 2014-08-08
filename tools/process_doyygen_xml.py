#! /usr/bin/python
from xml.dom import minidom
from sys import argv

def getText(node):
    rc = []
    for node in node.childNodes:
        if node.nodeType == node.TEXT_NODE:
            rc.append(node.data)
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

def report(publish, symbol):
    if publish: print "  PUBLISH:", symbol
    else      : print "NOPUBLISH:", symbol

def parseMemberDef(node):
    (kind, static, prot) = getAttribs(node)
    texttags = ['definition']
    if kind == 'function': texttags.append('argsstring')
    symbol = concatTextFromTags(node, texttags)
    publish = '/include/' in getLocationFile(node)
    if publish: publish = prot != 'private'
    if publish: publish = kind == 'function' or static == 'yes'
    report(publish, symbol)
    printAttribs(node, ['kind', 'prot', 'static'])
    printLocation(node)
    print

def parseCompoundDefs(xmldoc):
    compounddefs = xmldoc.getElementsByTagName('compounddef') 
    for node in compounddefs :
        kind = node.attributes['kind'].value

        if kind in ['page', 'file']: continue

        if kind in ['class', 'struct']:
            symbol = concatTextFromTags(node, ['compoundname'])
            prot =  node.attributes['prot'].value
            publish = '/include/' in getLocationFile(node)
            if publish: publish = prot != 'private'
            report(publish, symbol)
            printAttribs(node, ['kind', 'prot'])
            printLocation(node)
            print

        for member in node.getElementsByTagName('memberdef') : 
            parseMemberDef(member)

if __name__ == "__main__":
    for arg in argv[1:]:
        try:
            xmldoc = minidom.parse(arg)
            parseCompoundDefs(xmldoc)
        except Exception as error:
            print 'Error:', arg, error
