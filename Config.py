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

from lxml import objectify, etree
import xml.etree.ElementTree
import sys, logging, os

global logger

class Config:

    xml = ''

    def getCommandsFromMatchDescription(self, desc, event):
        if event:
            getDo = '/Config/Match[@description = "' + desc + '"]//Event[@value = "' + event + '"]/Do'
        else:
            getDo = '/Config/Match[@description = "' + desc + '"]//Do'
        logger.debug('xpath(' + getDo + ')')
        return self.xml.xpath(getDo)

    def getMatchFromEvent(self, event):
        return self.xml.xpath('/Config/Match/Event[@value = "' + event + '"]')

    def getCharacteristicFromSignal(self, signal):
        for i in self.xml.xpath('/Config/Match[Event[@value = "' + signal + '"]]/Characteristic'):
            #TODO
            xml.etree.ElementTree.dump(i)

    def __init__(self, configfile, loglevel):
        global logger
        logger = logging.getLogger('config')
        logger.setLevel(loglevel)

        program_basedir = os.path.dirname(sys.argv[0])

        #self.configO = objectify.parse(configfile).getroot()
        #mxml = self.configO.getchildren()
        #matchlist = Config.Match( mxml )
        
        # dtd needed to generate ref->idRef substitutions (cross-references)
        parser = etree.XMLParser( load_dtd=True )
        self.xml = etree.parse(configfile, parser)
        xslt1_path = program_basedir + '/xsl/idref2ref.xsl'

        if not os.path.isfile(xslt1_path):
            logging.error(xslt1_path + ": xslt doesn't exist")
            sys.exit(1)
        xslt_doc = etree.parse(xslt1_path)

        transformCharacteristics = etree.XSLT(xslt_doc)
        self.xml = transformCharacteristics(self.xml)

        tmpdir_path = program_basedir + '/tmp/conf.xml'
        f = open(tmpdir_path, 'w')
        if f:
            logger.info('created tmp/conf.xml to store processed configuration')
        else:
            logger.info('can not create "conf.xml", please be sure a writable tmp/ directory exists in the program directory')
            sys.exit(1)
        f.write(str(self.xml))
        f.close()

