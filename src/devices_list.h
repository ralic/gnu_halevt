/*
    devices_list.h declarations for devices_list.c
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

#ifndef DEVICES_LIST_H
#define DEVICES_LIST_H

#include <dbus/dbus.h>
#include <libhal.h>

typedef struct halevt_device_property
{
  char *key;
  char **values;
  struct halevt_device_property *next;
} halevt_device_property;

typedef struct halevt_device
{
   char *udi;
   struct halevt_device_property *properties;
   struct halevt_device *next;
} halevt_device;

halevt_device *halevt_device_root;

halevt_device *halevt_device_list_add_device (LibHalContext *ctx, const char *udi);
int halevt_device_list_remove_device (const char *udi);
int halevt_device_list_remove_property (const char *udi, const char *key);
halevt_device *halevt_device_list_find_device (const char *udi);
int halevt_device_list_set_property (const char *udi, const char *key);
halevt_device_property *halevt_device_list_get_property (const char *property,
   const halevt_device *device);
/* debugging */
void halevt_print_device (const halevt_device *device);
int halevt_count_devices ();

#endif
