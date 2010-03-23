/*
    public interface to interact with HAL
    Copyright (C) 2007  Patrice Dumas <pertusus at free dot fr>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef _HAL_INTERFACE_H
#define _HAL_INTERFACE_H

#include <libhal.h>
#include <dbus/dbus.h>

#include "devices_list.h"
#include "match.h"

void halevt_setup_HAL();
void halevt_cleanup_HAL();

void halevt_check_dbus_error(DBusError *error);

void halevt_run_oninit();

int halevt_matches (const halevt_match* match, const char *udi,
   const halevt_device *device);
char **halevt_udi_property_value (const char* property, const char *udi);
char **halevt_get_property_value(LibHalPropertyType type, const char *property,
   const char *udi, DBusError *dbus_error_pointer);
char **halevt_get_iterator_value(const LibHalPropertyType type,
  LibHalPropertySetIterator *iter);

#endif
