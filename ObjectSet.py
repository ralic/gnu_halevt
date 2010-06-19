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

from DbusObject import DbusObject
import os

class ObjectSet():

    objectset = {}
    iface = dbus = dbus = dbusaddr = objectsNames = None
    
    def __init__(self, iface, dbus, bus, dbusaddr, objnames ):
        self.iface = iface
        self.dbus = dbus
        self.bus = bus
        self.dbusaddr = dbusaddr
        self.objectsNames = objnames

    def rebuild(self):
        # remove duplicates
        for obj in list(set(self.objectsNames)):
            fname = 'Enumerate' + obj.capitalize() + 's'
            # dynamic method name
            for i in getattr(self.iface, fname)():
                path = self.bus.get_object(self.dbusaddr , i)
                prop_iface = self.dbus.Interface( path, "org.freedesktop.DBus.Properties" )
                all_props = prop_iface.GetAll( self.dbusaddr + '.' + obj )

                dbusobject = DbusObject ( i, all_props )
                # path (org.freedesktop....., im.ekiga, ...) assumed unique
                self.objectset[i] = dbusobject

    # shortcut
    def get(self, objectpath):
        return self.objectset.get(objectpath)

    def dump_to_file(self, afile):
        if os.path.isfile(afile):
            return #don't overwrite anything

        # manual debug:
        # sed -i -e '5,$s;<.pyxslt>;;' -e '5,$s;<?xml version="1.0" encoding="ASCII"?>;;' -e '$a</pyxslt>' tmp/objdump.xml
        # xsltproc --stringparam config_filepath ../tmp/conf.xml xsl/comparator.xsl tmp/objdump.xml
        f = open(afile, 'w')
        for i in self.objectset.keys():
            f.write("<!-- ======== -->\n" + self.get(i).xml)
        f.close()
