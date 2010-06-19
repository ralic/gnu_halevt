# Copyright (C) 2010 Raphael Droz <raphael.droz@gmail.com>

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#from pyxslt.serialize import Serializer
import pyxslt.serialize

from lxml import objectify, etree
import xml.etree.ElementTree
import lxml.etree

import os, sys

# Dbus should provide an XML output
class DbusObject:
    path = xml = ''
    props = {}

    def __init__(self, path, dictionary):
        self.path = path
        self.props = dictionary
        self.genXML()

    def genXML(self):
        self.xml = {}
        a = {}
        # TODO; better to directly send the dict (and strip DriveAtaSmartBlob), if possible
        for i in self.props.keys():
            # NULL Bytes :(
            if i == 'DriveAtaSmartBlob':
                continue
            if type(self.props.get(i)).__name__ == 'Array':
                a[i] = self.atostring(self.props.get(i))
            else:
                a[i] = self.props.get(i)
        # TODO; better to add @name in the obj root-node itself (if possible) rather than adding
        # an element with an arbitrary name
        a['_pyxslt_dbus_path'] = self.path
        self.xml = pyxslt.serialize.toString(prettyPrintXml=True, obj=a)
        # sadly following doesn't work (python experts welcome)
        # self.xml = pyxslt.serialize.toString(prettyPrintXml=True, foo=self.props)

    # helper for hand-serialization
    def atostring(self, arg):
        if type(arg).__name__ == 'Array':
            a = []
            for i in arg:
                a.append(self.atostring(i))
            return a
        else:
            return arg
        
    def processAgainstConfig(self, config):
        xml = etree.fromstring(self.xml)

        xslt_doc = etree.parse(os.path.dirname(sys.argv[0]) + '/xsl/comparator.xsl')
        transformCharacteristics = etree.XSLT(xslt_doc)
        res = transformCharacteristics(xml,
                                       config_filepath = etree.XSLT.strparam(config))
        #TODO: here we should handle BooleanProperties
        if res.xpath('count(/results/match)') == 0:
            return None
        return res
