#!/usr/bin/python
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

# The main program: works with dbus signals
# After a configuration file is loaded, it attach to specified dbus services
# When an event caught happens:
# - spawn specified actions
# - refresh its internal list of objects/properties

import dbus, gobject, inspect, string

from dbus.mainloop.glib import DBusGMainLoop

# used in doit(), command_substitute()
import re, os, sys

# for args handling
import sys, getopt

# verbosity
import logging

import Config
from ObjectSet import ObjectSet

LEVELS = {'debug': logging.DEBUG,
          'info': logging.INFO,
          'warning': logging.WARNING,
          'error': logging.ERROR}

DEFAULT_CONFIG_FILE_NAME='.dbusevt.xml'

signals = []
config = iface = dbusobjects = None

DRYRUN = False

# whatever the attached signal name is
# processEvent() applies
def attach_signal(signal):
    global signals, iface

    if signal not in signals:
        signals.append ( signal )
        logging.debug("attach to %s" % (signal))
        # tricky: in order to grab both the return value + the signal:
        # we dynamically create a function which will always pass the signal name as
        # first argument to processEvent()
        # The function name itself is dynamic to avoid namespace overlapping
        exec ("def dbus_callback_"+signal+"(dbusobject):\n\tprocessEvent('" + signal + "', dbusobject)", globals())
        exec ("iface.connect_to_signal(signal, dbus_callback_"+signal+")")

def doit(nodesToRun, dbusobject, event):
    for i in nodesToRun :
        if i.get('type') == 'script':
            torun = i.get('value')
            torun = re.sub('\$native_path\$', dbusobject, torun)
            torun = re.sub('\$event\$', event, torun)
            torun = command_substitute(torun, dbusobject)

            if DRYRUN or i.get('fork') != 'true':
                logging.info("spawn: " + torun)

            if not DRYRUN:
                if i.get('fork') == 'true':
                    pid = os.spawnlp(os.P_NOWAIT, '/bin/bash', 'bash', '-c', torun)
                    logging.info("spawn (pid %s): %s" % (pid, torun) )
                else:
                    os.spawnlp(os.P_WAIT, '/bin/bash', 'bash', '-c', torun)
                
        elif i.get('type') == 'binary': #todo: args = list()
            if i.get('arg'):
                args = i.get('args')
                args = re.sub('\$native_path\$', dbusobject, args)
                args = re.sub('\$event\$', event, args)
                args = command_substitute(args, dbusobject)
                torun_arr = args.split
                torun_arr.insert(0, i.get('value'))
            else:
                args = []
            if not DRYRUN:
                pid = os.spawnvp(os.P_NOWAIT, i.get('value'), torun_arr)
                logging.info("spawn (pid %s): %s %s" % (pid, i.get('value'), args) )
            else:
                logging.info("spawn: %s %s" % (i.get('value'), torun_arr) )
        else:
            #TODO
            logging.info("here we are " + torun)

# ret is often (always ?) the freedesktop resource name
def processEvent(event, ret):
    global dbusobjects, config

    logging.info( '%s happened: got %s' % ( event, ret ) )

    # if "removed", process BEFORE
    # TODO: later: don't rebuild the whole as we know event & ret
    dbusobjects.rebuild()

    # otherwise process AFTER rebuilt
    logging.debug("matching props in Config :")
    for i in config.getMatchFromEvent( event ):
        print str(i)

    logging.debug("running processObject() with " + ret)
    processObject(config, dbusobjects.get(ret), event)


## TODO: most of that xpath should move to XSLT itself (directly returns <Do>)
## TODO: should use fn;collection in comparator.xsl
## (the day xsltproc supports it)
def processObject(config, dbusobject, event = None):
    #TODO: hardcoded path ( event worst: relative to xsl/ )
    result = dbusobject.processAgainstConfig('../tmp/conf.xml')
    if not result:
        return

    for i in result.xpath('/results/match'):
        doit(config.getCommandsFromMatchDescription( i.get('description'), event ), 
             i.get('dbusObject'),
             event)

def command_substitute(torun, dbusobject):
    global dbusobjects

    for j in re.findall('\$.*?\$', torun):
        aprop = dbusobjects.get(dbusobject).props.get( j.strip('$') )
        if aprop == None:
            aprop = 'DBUSEVT_UNKNOWN_PROP'
        torun = re.sub('\$' + j.strip('$') + '\$',
                       str(aprop),
                       torun)
    return torun

def usage():
    usage = """%s -h | [-c config] [-n] [-v verbosity]

Listen to dbus events the configuration file and trigger specified commands accordingly

-h|--help\t\tprint usage
-n|--dry-run\t\tdo not really runs commands
-v|--verbosity\t\tset the verbosity level (debug|info|warning|error)
-c|--config file\tspecify the configuration file to use
\t\t\t(if it fails, an attempt will be made with ~/%s)
"""
    print usage % (os.path.basename(sys.argv[0]), DEFAULT_CONFIG_FILE_NAME)

def main(argv):
    try:
        opts, args = getopt.getopt(argv, "hv:c:n", ["help", "verbosity=", "config", "dry-run"])
    except getopt.GetoptError:
        usage()
        sys.exit(2)

    global DRYRUN, config, iface, dbusobjects
    loglevel = logging.ERROR
    config_path = ''

    for opt, arg in opts:
        if opt in ('-h', '--help'):
            usage()
            sys.exit()
        elif opt in ('-n', '--dry-run'):
            DRYRUN = True
        elif opt in ("-v", "--verbosity"):
            loglevel = LEVELS.get(arg, logging.NOTSET)
        elif opt in ("-c", "--config"):
            config_path = arg

    logging.basicConfig(level=loglevel)

    if not os.path.isfile(config_path):
        fallback = os.path.expanduser('~') + '/' + DEFAULT_CONFIG_FILE_NAME
        if os.path.isfile(fallback):
            config_path = fallback
        else:
            logging.error('no configuration file found under "' + config_path + '"')
            sys.exit(1)

    if DRYRUN:
        logging.info('DRY-RUN mode, no command will be run')

    logging.debug('loading config: ' + config_path)
    config = Config.Config( config_path, loglevel )
           
    # TODO: handle multiple Config service (udisks, upower)
    namespace = config.xml.xpath('/Config/@namespace')[0]
    service = config.xml.xpath('/Config/@interface')[0]
    objnames = config.xml.xpath('/Config/Match/@object')
    dbusaddr = namespace + '.' + service

    DBusGMainLoop(set_as_default=True)
    bus = dbus.SystemBus()

    proxy = bus.get_object(dbusaddr ,
                           '/' + string.replace(namespace, '.', '/') + '/' + service )
    iface = dbus.Interface(proxy, dbusaddr )
    
    for i in config.xml.xpath('/Config/Match//Event[@value != "OnInit"]'):
        # TODO: check if this signal exists according to its <Match@object>
        attach_signal(i.get('value'))

    dbusobjects = ObjectSet(iface, dbus, bus, dbusaddr, objnames)
    dbusobjects.rebuild()

    # dump dbus serialization in debug mode
    if loglevel == logging.DEBUG:
        dbusobjects.dump_to_file(os.path.dirname(sys.argv[0]) + '/tmp/objdump.xml')

    # initial matching
    for i in dbusobjects.objectset.keys():
        processObject(config, dbusobjects.get(i), 'OnInit')

    mainloop = gobject.MainLoop()
    mainloop.run()

if __name__ == "__main__":
    main(sys.argv[1:])
