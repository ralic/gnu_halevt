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

void halevt_free_property_value (char **values)
{
    char **cur_value = values;
    
    while ((*cur_value) != NULL)
    {
        free (*cur_value);
        cur_value++;
    }
    free (values);
}

void halevt_free_device_property (halevt_device_property *property)
{
    free (property->key);
    halevt_free_property_value (property->values);
}

void halevt_free_device (halevt_device *device)
{
    halevt_device_property *property = device->properties;
    halevt_device_property *previous_property;

    free (device->udi);
    
    while (property != NULL)
    {
        previous_property = property;
        property = property->next;
        free (previous_property);
    }
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
   LibHalPropertySetIterator *device_property_iterator;
   LibHalPropertySet* device_property_set;
   halevt_device *device;
   char *key;
   char *udi_string;
   char **value;
   halevt_device_property *new_property;

   udi_string = strdup(udi);
   if (udi_string == NULL) { return NULL; }

   device_property_iterator = malloc(sizeof(device_property_iterator));
   if (device_property_iterator == NULL) { return NULL; }

   dbus_error_init(&dbus_error);
   device_property_set = libhal_device_get_all_properties(ctx, udi, &dbus_error);
   halevt_check_dbus_error (&dbus_error);
   if (device_property_set == NULL)
   {
      DEBUG(_("No property found for %s (or oom)"), udi);
      return NULL;
   }
   libhal_psi_init(device_property_iterator, device_property_set);
   if (device_property_iterator == NULL)
   {
      DEBUG(_("Iterator initialization failed for %s (or oom)"), udi);
      return NULL;
   }
   
   device = malloc (sizeof(halevt_device));
   if (device == NULL) { return NULL; }
   device->udi = udi_string;
   device->properties = NULL;

   while (libhal_psi_has_more(device_property_iterator))
   {
      LibHalPropertyType type = libhal_psi_get_type(device_property_iterator);
      key = strdup(libhal_psi_get_key(device_property_iterator));
      if (key == NULL) { goto oom; }
      value = halevt_get_iterator_value (type, device_property_iterator);
      if (value == NULL) { goto oom; }
      new_property = halevt_new_device_property (key, value);
      if (new_property == NULL) { goto oom; }
      new_property->next = device->properties;
      device->properties = new_property;

      libhal_psi_next(device_property_iterator);
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
   halevt_device *previous = halevt_device_root;
   halevt_device *device = halevt_device_root;

   /* no device. Assert? */
   if (device == NULL) { return 0; };

   /* device is the first one */
   if (!strcmp (device->udi, udi))
   {
      halevt_device_root = device->next;
      halevt_free_device(device);
      return 1;
   }

   while (device != NULL)
   {
      if (!strcmp (device->udi, udi)) 
      {
          previous->next = device->next;
          halevt_free_device(device);
          return 1;
      }
      previous = device;
      device = device->next;
   }
   return 0;
}

halevt_device *halevt_device_list_find_device(const char *udi)
{
   halevt_device *device = halevt_device_root;
   while (device != NULL)
   {
      if (!strcmp (device->udi, udi)) { return device; };
      device = device->next;
   }
   return NULL;
}

halevt_device_property *halevt_device_list_get_property (const char *key, 
   const halevt_device *device)
{
   halevt_device_property *property = device->properties;

   while (property != NULL)
   {
       if (!strcmp (key, property->key))
       {
           return property;
       }
       property = property->next;
   }
   return NULL;
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
       halevt_free_property_value(property->values);
       property->values = values;
   }
   else
   {
       char *new_key = strdup(key);
       if (new_key == NULL)
       {
           halevt_free_property_value(values);
           return 0; 
       }
       property = halevt_new_device_property (new_key, values);
       if (property == NULL) 
       {
           halevt_free_property_value(values);
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
   halevt_device_property *property;
   halevt_device_property *previous;

   if (device == NULL) { return 0; }

   /* device without properties. Assert? */
   if (device->properties == NULL) { return 0; }

   property = device->properties;
   /* first property */
   if (!strcmp(key, property->key))
   {
       device->properties = property->next;
       halevt_free_device_property (property);
   }
   previous = property;

   while (property != NULL)
   {
      if (!strcmp (property->key, key)) 
      {
          previous->next = property->next;
          halevt_free_device_property(property);
          return 1;
      }
      previous = property;
      property = property->next;
   }
   return 0;
}

/* debugging */
void halevt_print_device (const halevt_device *device)
{
    halevt_device_property *property = device->properties;
    char **cur_value;

    if (device == NULL) { fprintf(stderr, "Device is NULL\n"); }

    fprintf (stderr, "Device udi: %s\n", device->udi);
    while (property != NULL)
    {
        fprintf(stderr, " %s = ", property->key);
        cur_value = property->values;
        while ((*cur_value) != NULL)
        {
            fprintf (stderr, "'%s' ", (*cur_value));
            cur_value++;
        }
        fprintf (stderr, "\n");
        property = property->next;
    }
}

void halevt_print_all_devices ()
{
   halevt_device *device = halevt_device_root;
   while (device != NULL)
   {
      fprintf(stderr, "%s\n", device->udi);
      device = device->next;
   }
}

int halevt_count_devices ()
{
   int nr = 0;
   halevt_device *device = halevt_device_root;
   while (device != NULL)
   {
      nr++;
      device = device->next;
   }
   return nr;
}
