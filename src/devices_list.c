/*
    devices_list.c handle device list that keeps the state of all the
    devices.
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

#include "common.h"
#include "devices_list.h"
#include "hal_interface.h"
#include <libintl.h>
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>
#include <libhal.h>

void halevt_free_device_property (halevt_device_property *property)
{
    free (property->key);
    FREE_NULL_ARRAY(char *, property->values, free);
}

void halevt_free_device (halevt_device *device)
{
    free (device->udi);
    FREE_LINKED_LIST(halevt_device_property, device->properties, free);
}

halevt_device_property *halevt_new_device_property (char *key,
   char **values)
{
   halevt_device_property *new_property = malloc (sizeof(halevt_device_property));
   if (new_property == NULL) { return NULL; }
   new_property->values = values;
   new_property->key = key;
   return new_property;
}

halevt_device *halevt_device_list_add_device (LibHalContext *ctx, const char *udi)
{
   DBusError dbus_error;
   LibHalPropertySetIterator device_property_iterator;
   LibHalPropertySet* device_property_set;
   halevt_device *device;
   char *key;
   char *udi_string;
   char **value;
   halevt_device_property *new_property;

   udi_string = strdup(udi);
   if (udi_string == NULL) { return NULL; }

   dbus_error_init(&dbus_error);
   device_property_set = libhal_device_get_all_properties(ctx, udi, &dbus_error);
   halevt_check_dbus_error (&dbus_error);
   if (device_property_set == NULL)
   {
      DEBUG(_("No property found for %s (or oom)"), udi);
      return NULL;
   }

   device = malloc (sizeof(halevt_device));
   if (device == NULL) { return NULL; }
   device->udi = udi_string;
   device->properties = NULL;

   for (libhal_psi_init(&device_property_iterator, device_property_set);
        libhal_psi_has_more(&device_property_iterator);
        libhal_psi_next(&device_property_iterator))
   {
      LibHalPropertyType type = libhal_psi_get_type(&device_property_iterator);
      key = strdup(libhal_psi_get_key(&device_property_iterator));
      if (key == NULL) { goto oom; }
      value = halevt_get_iterator_value (type, &device_property_iterator);
      if (value == NULL) { goto oom; }
      new_property = halevt_new_device_property (key, value);
      if (new_property == NULL) { goto oom; }
      new_property->next = device->properties;
      device->properties = new_property;
   }
   device->next = halevt_device_root;
   halevt_device_root = device;
   return device;

oom:
   halevt_free_device(device);
   return NULL;
}

int halevt_device_list_remove_device (const char *udi)
{
   halevt_device *previous;
   halevt_device *device;

   /* no device. Assert? */
   if (halevt_device_root == NULL) { return 0; };

   WALK_LINKED_LISTP(previous, device, halevt_device_root)
   {
      if (!strcmp (device->udi, udi))
      {
          if (previous == NULL) { halevt_device_root = device->next; }
          else { previous->next = device->next; }
          halevt_free_device(device);
          return 1;
      }
   }
   return 0;
}

halevt_device *halevt_device_list_find_device(const char *udi)
{
   halevt_device *device;

   WALK_LINKED_LIST(device, halevt_device_root)
   {
      if (!strcmp (device->udi, udi)) { break; }
   }

   return device;
}

halevt_device_property *halevt_device_list_get_property (const char *key,
   const halevt_device *device)
{
   halevt_device_property *property;

   WALK_LINKED_LIST(property, device->properties)
   {
       if (!strcmp (key, property->key)) { break; }
   }

   return property;
}

int halevt_device_list_set_property (const char *udi, const char *key)
{
   halevt_device *device = halevt_device_list_find_device(udi);
   halevt_device_property *property;

   char **values;

   if (device == NULL) { return 0; }

   values = halevt_property_value (key, udi, NULL);
   if (values == NULL) { return 0; }

   property = halevt_device_list_get_property(key, device);
   if (property != NULL)
   {
       FREE_NULL_ARRAY(char *, property->values, free);
       property->values = values;
   }
   else
   {
       char *new_key = strdup(key);
       if (new_key == NULL)
       {
           FREE_NULL_ARRAY(char *, values, free);
           return 0;
       }
       property = halevt_new_device_property (new_key, values);
       if (property == NULL)
       {
           FREE_NULL_ARRAY(char *, values, free);
           free(new_key);
           return 0;
       }
       property->next = device->properties;
       device->properties = property;
   }
   return 1;
}

int halevt_device_list_remove_property (const char *udi, const char *key)
{
   halevt_device *device = halevt_device_list_find_device(udi);
   halevt_device_property *previous;
   halevt_device_property *property;

   if (device == NULL) { return 0; }

   /* device without properties. Assert? */
   if (device->properties == NULL) { return 0; }

   WALK_LINKED_LISTP(previous, property, device->properties)
   {
      if (!strcmp (property->key, key))
      {
          if (previous == NULL) { device->properties = property->next; }
          else { previous->next = property->next; }
          halevt_free_device_property(property);
          return 1;
      }
   }

   return 0;
}

/* debugging */
void halevt_print_device (const halevt_device *device)
{
    halevt_device_property *property;

    if (device == NULL) { fprintf(stderr, "Device is NULL\n"); }

    fprintf (stderr, "Device udi: %s\n", device->udi);

    WALK_LINKED_LIST(property, device->properties)
    {
        char **cur_value;

        fprintf(stderr, " %s = ", property->key);

        WALK_NULL_ARRAY(cur_value, property->values)
        {
            fprintf (stderr, "'%s' ", (*cur_value));
        }

        fprintf (stderr, "\n");
    }
}

void halevt_print_all_devices ()
{
   halevt_device *device;

   WALK_LINKED_LIST(device, halevt_device_root)
   {
      fprintf(stderr, "%s\n", device->udi);
   }
}

int halevt_count_devices ()
{
   int nr = 0;
   halevt_device *device;

   WALK_LINKED_LIST(device, halevt_device_root) { nr++; }

   return nr;
}
