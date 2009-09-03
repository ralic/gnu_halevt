/*
    hal_interface.c register callbacks to hal, check matching hal
       properties and launch commands with hal properties substituted
    Copyright (C) 2007  Patrice Dumas <pertusus at free dot fr>
    (C) Copyright 2005 Novell, Inc.

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

/*
 * The (C) Copyright 2005 Novell, Inc. is for halevt_setup_HAL, inspired from
 * code in gnome-volume-manager src/manager.c, with author Robert Love
 * <rml@novell.com>
 */

#include <dbus/dbus.h>
#include <glib.h>
#include <libhal.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <stdlib.h>
#include <string.h>
#include <libintl.h>

#include "common.h"
#include "match.h"
#include "parse_config.h"
#include "hal_interface.h"
#include "devices_list.h"

static LibHalContext *hal_ctx;

/** Invoked when a device is added to the Global Device List.
*
*  @param  udi                 Universal Device Id
*/
void halevt_device_added(LibHalContext *ctx, const char *udi)
{
    halevt_insertion *current_insertion = halevt_insertion_root;

    DEBUG_REPORT("New Device: %s", udi);
    while (current_insertion != NULL)
    {
        if (halevt_true_tree(current_insertion->match, udi, NULL))
        {
            halevt_run_command(current_insertion->exec, udi, NULL);
        }
        current_insertion = current_insertion->next;
    }
    /* sync the devices list */
    halevt_device_list_add_device (ctx, udi);
}

/** Invoked when a device is removed from the Global Device List.
*
*  @param  udi                 Universal Device Id
*/
void halevt_device_removed(LibHalContext *ctx, const char *udi)
{
    halevt_removal *current_removal = halevt_removal_root;
    halevt_device *removed_device;

    DEBUG_REPORT("Device removed: %s", udi);

    /* use preferably the device from the devices list since the
     * properties will be right */
    removed_device = halevt_device_list_find_device(udi);
    if (removed_device != NULL)
    {
        while (current_removal != NULL)
        {
            if (halevt_true_tree(current_removal->match, NULL, removed_device))
            {
                halevt_run_command(current_removal->exec, udi, removed_device);
            }
            current_removal = current_removal->next;
        }
        /* sync the devices list */
        halevt_device_list_remove_device(udi);
    }
    else
    {
        DEBUG("Cannot find device %s in halevt list", udi);
        /* FIXME resync devices? */
        while (current_removal != NULL)
        {
            if (halevt_true_tree(current_removal->match, udi, NULL))
            {
                halevt_run_command(current_removal->exec, udi, NULL);
            }
            current_removal = current_removal->next;
        }
    }
}

/** Invoked when device in the Global Device List acquires a new capability.
*
*  @param  udi                 Universal Device Id
*  @param  capability          Name of capability
*/
void halevt_device_new_capability(LibHalContext *ctx, const char *udi,
                        const char *capability)
{
    DEBUG_REPORT("Capability new %s:%s",udi,capability);
}

/** Invoked when device in the Global Device List loses a capability.
*
*  @param  udi                 Universal Device Id
*  @param  capability          Name of capability
*/
void halevt_device_lost_capability (LibHalContext *ctx, const char *udi,
                           const char *capability)
{
    DEBUG_REPORT("Capability lost %s:%s",udi,capability);
}

/** Invoked when a property of a device in the Global Device List is
*  changed, and we have we have subscribed to changes for that device.
*
*  @param  udi                 Univerisal Device Id
*  @param  key                 Key of property
*/
void halevt_device_property_modified(LibHalContext *ctx, const char *udi,
  const char *key, dbus_bool_t is_removed, dbus_bool_t is_added)
{
    DEBUG_REPORT("Property %s:%s modified (removed: %d, added: %d)",udi,key,is_removed,is_added);

    halevt_property *current_property = halevt_property_root;
    halevt_property_action *current_action;
    while (current_property != NULL)
    {
       if (!strcmp(current_property->name, key) &&
           halevt_true_tree(current_property->match, udi, NULL))
       {
           current_action = current_property->action;
           while (current_action != NULL)
           {
               if ((!strcmp (current_action->value, "*")) || halevt_property_matches_udi(key, current_action->value, udi))
               {
                   halevt_run_command(current_action->exec, udi, NULL);
               }
               current_action = current_action->next;
           }
       }
       current_property = current_property->next;
    }
    /* now sync the properties in the devices list */
    if (is_removed)
    {
        halevt_device_list_remove_property (udi, key);
    }
    else
    {
        halevt_device_list_set_property (udi, key);
    }
}

/** Invoked when a device in the GDL emits a condition that cannot be
*  expressed in a property (like when the processor is overheating)
*
*  @param  udi                 Univerisal Device Id
*  @param  condition_name      Name of condition
*/
void halevt_device_condition(LibHalContext *ctx, const char *udi,
    const char *condition_name, const char *condition_detail)
{
    DEBUG_REPORT("Condition: %s,%s (%s)", udi,condition_name,condition_detail);

    halevt_condition *current_condition = halevt_condition_root;
    while (current_condition != NULL)
    {
        if (!strcmp(current_condition->name, condition_name)
            &&  (current_condition->value == NULL
                 ||  !strcmp(current_condition->value, condition_detail))
            &&  halevt_true_tree(current_condition->match, udi, NULL))
        {
            halevt_run_command(current_condition->exec, udi, NULL);
        }
        current_condition = current_condition->next;
    }
}

/*
 * Set up connection to HAL and set callbacks to the functions that will
 * be called for hal events.
 * Inspired from gvm_hal_init.
 */
void halevt_setup_HAL()
{
    DBusError dbus_error;
    DBusConnection *dbus_connection;

    if ((hal_ctx = libhal_ctx_new()) == NULL)
    {
        DEBUG(_("Failed to create HAL context"));
        exit(1);
    }

    dbus_error_init (&dbus_error);
    dbus_connection = dbus_bus_get (DBUS_BUS_SYSTEM, &dbus_error);
    if (dbus_error_is_set (&dbus_error))
    {
        DEBUG (_("Error connecting to D-BUS system bus: %s"), dbus_error.message);
        dbus_error_free (&dbus_error);
        exit(1);
    }
    dbus_connection_setup_with_g_main (dbus_connection, NULL);

    libhal_ctx_set_dbus_connection (hal_ctx, dbus_connection);

    libhal_ctx_set_device_added(hal_ctx, halevt_device_added);
    libhal_ctx_set_device_removed(hal_ctx, halevt_device_removed);
    libhal_ctx_set_device_new_capability(hal_ctx,
                                         halevt_device_new_capability);
    libhal_ctx_set_device_lost_capability(hal_ctx,
                                          halevt_device_lost_capability);
    libhal_ctx_set_device_property_modified(hal_ctx,
                                            halevt_device_property_modified);
    libhal_ctx_set_device_condition(hal_ctx, halevt_device_condition);

    if (!libhal_ctx_init(hal_ctx, &dbus_error))
    {
        DEBUG(_("Error initializing HAL"));
        exit(1);
    }
    if (!libhal_device_property_watch_all(hal_ctx, &dbus_error))
    {
        DEBUG(_("Failed to watch all HAL properties: %s"), dbus_error.message);
        exit(1);
    }
}

/*
 * print and reset a dbus error
 */
void halevt_check_dbus_error(DBusError *error)
{
    if (dbus_error_is_set(error))
    {
        DEBUG(_("D-Bus Error %s: %s"), error->name, error->message);
        dbus_error_free(error);
    }
}

/*
 * run the commmands for each of the devices on startup
 */
void halevt_run_oninit()
{
   DBusError dbus_error;
   char **all_udi;
   char **current_udi;
   int num_device;

   dbus_error_init(&dbus_error);
   all_udi = libhal_get_all_devices (hal_ctx, &num_device, &dbus_error);
   halevt_check_dbus_error (&dbus_error);

   if (all_udi == NULL)
   {
       DEBUG(_("No hal devices. Out of memory or an error occured in Hal"));
       return;
   }
   
   WALK_NULL_ARRAY(current_udi, all_udi)
   {
      halevt_oninit *current_oninit;

/*
      DEBUG("Run OnInit for device: %s", (*current_udi));
*/

      WALK_LINKED_LIST(current_oninit, halevt_oninit_root)
      {
         if (halevt_true_tree(current_oninit->match, (*current_udi), NULL))
         {
             halevt_run_command(current_oninit->exec, (*current_udi), NULL);
         }
      }
      /* construct the devices list */
      halevt_device_list_add_device (hal_ctx, *current_udi);
   }

   libhal_free_string_array(all_udi);
}

/*
 * return true if a property of a device specified by its udi
 * match a given value.
 */
int halevt_property_matches_udi (const char *property,
                         const char *value, const char *udi)
{
   DBusError dbus_error;
   int result = 0;

   if ((udi == NULL) || (property == NULL))
   {
      DEBUG("Warning: halevt_property_matches called with a NULL value");
      return 0;
   }

   dbus_error_init(&dbus_error);

   if (libhal_device_property_exists (hal_ctx, udi, property, &dbus_error))
   {
      LibHalPropertyType type;

      halevt_check_dbus_error (&dbus_error);

      if (value == NULL) { return 1; }

      type = libhal_device_get_property_type
           (hal_ctx, udi, property, &dbus_error);
      halevt_check_dbus_error (&dbus_error);

      if (type == LIBHAL_PROPERTY_TYPE_BOOLEAN)
      {
          dbus_bool_t val_bool = FALSE;
          if (! strcmp(value, "true")) { val_bool = TRUE; }
          result = (libhal_device_get_property_bool
               (hal_ctx, udi, property, &dbus_error) == val_bool);
      }
      else if (type == LIBHAL_PROPERTY_TYPE_STRING)
      {
          char *str_val = libhal_device_get_property_string
               (hal_ctx, udi, property, &dbus_error);
          if (str_val != NULL)
          {
             result = (! strcmp (value, str_val));
             libhal_free_string (str_val);
          }
      }
      else if (type == LIBHAL_PROPERTY_TYPE_INT32)
      {
          result = (libhal_device_get_property_int
               (hal_ctx, udi, property, &dbus_error) == atoi(value));
      }
      else if (type == LIBHAL_PROPERTY_TYPE_STRLIST)
      {
          /* impulze: why was this LIBHAL_PROPERTY_TYPE_STRING? */
          char **cur_str;
          char **str_list = libhal_device_get_property_strlist
                (hal_ctx, udi, property, &dbus_error);
          WALK_NULL_ARRAY(cur_str, str_list)
          {
              if (!strcmp((*cur_str), value))
              {
                  result = 1;
                  break;
              }
          }

          libhal_free_string_array (str_list);
      }
      else if (type == LIBHAL_PROPERTY_TYPE_UINT64)
      {
          result = (libhal_device_get_property_int
               (hal_ctx, udi, property, &dbus_error) ==
                  strtoull(value, NULL, 10));
      }
      else if (type == LIBHAL_PROPERTY_TYPE_DOUBLE)
      {
          result = (libhal_device_get_property_double
               (hal_ctx, udi, property, &dbus_error) ==
                  strtod(value, NULL));
      }
      else
      {
         DEBUG(_("Hal type not handled in match (%s,%s,%s): %d"),
            property, value, type);
         return 0;
      }
   }

   halevt_check_dbus_error (&dbus_error);

   return result;
}

/*
 * return true if a property of a device specified by its device
 * match a given value.
 */
int halevt_property_matches_device (const char *key,
                         const char *value, const halevt_device *device)
{
   halevt_device_property *property;
   char **values = NULL;

   if ((device == NULL) || (key == NULL))
   {
      DEBUG("Warning: halevt_property_matches_device called with a NULL value");
      return 0;
   }

   if ((property = halevt_device_list_get_property (key, device)) == NULL) { return 0; }

   if (value == NULL) { return 1; }

   WALK_NULL_ARRAY(values, property->values)
   {
       if (!strcmp(*values, value)) { return 1; }
   }

   return 0;
}

int halevt_match_device (const halevt_match *match, const halevt_device *device)
{
    const halevt_device *new_device = device;
    const halevt_device_property *udi_property;
    char **new_udi;
    char **parent;

    WALK_NULL_ARRAY(parent, match->parents)
    {
       udi_property = halevt_device_list_get_property (*parent, new_device);
       if (udi_property != NULL)
       {
          new_udi = udi_property->values;
          new_device = halevt_device_list_find_device(new_udi[0]);
          if (new_device == NULL) { return 0 ;}
       }
       else
       {
          return 0;
       }
    }
    return halevt_property_matches_device(match->name, match->value, new_device);
}

int halevt_match_udi (const halevt_match *match, const char *udi)
{
    DBusError dbus_error;
    const char *new_udi = udi;
    char **parent;

    dbus_error_init(&dbus_error);

    WALK_NULL_ARRAY(parent, match->parents)
    {
       if (! libhal_device_property_exists
             (hal_ctx, new_udi, *parent, &dbus_error))
       {
          return 0;
       }
       halevt_check_dbus_error(&dbus_error);

       new_udi = libhal_device_get_property_string
            (hal_ctx, new_udi, (*parent), &dbus_error);
       halevt_check_dbus_error(&dbus_error);
    }
    return halevt_property_matches_udi(match->name, match->value, new_udi);
}

/*
 * return true if a device matches a halevt_match
 */
int halevt_matches (const halevt_match *match, const char *udi,
   const halevt_device *device)
{
    if (((udi == NULL) && (device == NULL)) || (match == NULL))
    {
        DEBUG("Warning: hal_matches was called with a NULL value");
        return 0;
    }

    if (!strcmp (match->name, "*")) { return 1; }

    if (udi != NULL) { return halevt_match_udi(match, udi) ;}
    return (halevt_match_device (match, device));
}

/*
 * return the value of a property for a device specified by its udi
 */
char **halevt_udi_property_value (const char *property, const char *udi)
{
    char **values = NULL;

    DBusError dbus_error;

    if ((udi == NULL) || (property == NULL))
    {
        DEBUG("Warning: hal_udi_property_value was called with a NULL value");
        return NULL;
    }

    dbus_error_init(&dbus_error);

    if (libhal_device_property_exists(hal_ctx, udi, property, &dbus_error))
    {
        LibHalPropertyType type;

	halevt_check_dbus_error(&dbus_error);

	type = libhal_device_get_property_type(hal_ctx, udi, property, &dbus_error);
	halevt_check_dbus_error(&dbus_error);

        values = halevt_get_property_value(type, property, udi, &dbus_error);

    }
    halevt_check_dbus_error(&dbus_error);

    return values;
}

/*
 * return the value for a property. If the property is 'udi' simply
 * return the udi. Otherwise if a device is given look at the device
 * property, otherwise use hal to get the property.
 * -- halevt_property_value calls halevt_duplicate_str_list defined below
 * -- so declcare it here
 */
char **halevt_duplicate_str_list(char **);
char **halevt_property_value(const char *key, const char *udi,
   const halevt_device* device)
{
    char **values;

    if ((udi == NULL) || (key == NULL))
    {
        DEBUG("Warning: halevt_property_value was called with a NULL value");
        return NULL;
    }
    if (!strcmp(key,"udi"))
    {
        char *new_udi = strdup(udi);
        if (new_udi == NULL) { return NULL; }
        values = malloc(2*sizeof(char*));
        if (values == NULL)
        {
            free (new_udi);
            return NULL;
        }
        values[0] = new_udi;
        values[1] = NULL;
    }
    else if (device != NULL)
    {
        halevt_device_property *property =
             halevt_device_list_get_property(key, device);
        values = property != NULL ? halevt_duplicate_str_list(property->values) : NULL;
    }
    else
    {
        values = halevt_udi_property_value(key, udi);
    }

    return values;
}

char **halevt_duplicate_str_list(char** str_list)
{
   char **value = NULL;
   char **cur_str;

   if (str_list != NULL)
   {
       char *cur_val;
       unsigned int total_value_nr;
       char **cur_value;
       total_value_nr = libhal_string_array_length(str_list);

       value = malloc ((total_value_nr+1) * sizeof(char*));
       if (value == NULL)
       {
           return NULL;
       }
       *value = NULL;

       cur_value = value;

       WALK_NULL_ARRAY(cur_str, str_list)
       {
           cur_val = strdup(*cur_str);
           if (cur_val == NULL)
           {
               FREE_NULL_ARRAY(char *, value, free);
               return NULL;
           }
           *cur_value = cur_val;
           cur_value++;
           *cur_value = NULL;
       }
   }
   return value;
}

/* cut and paste of halevt_get_property_value with libhal_psi_get_* instead of
 * libhal_device_get_property_*
 * don't free hal resources here, the whole set needs to be free'd by the
 * caller via libhal_free_property_set() */
char **halevt_get_iterator_value(const LibHalPropertyType type,
  LibHalPropertySetIterator *iter)
{
    char **value = NULL;
    char tmp[256];

    if (type == LIBHAL_PROPERTY_TYPE_STRLIST)
    {
       char **str_list = libhal_psi_get_strlist (iter);
       return halevt_duplicate_str_list(str_list);
    }

    value = malloc(2*sizeof(char *));
    if (value == NULL) { return NULL; };
    value[1] = NULL;

    if (type == LIBHAL_PROPERTY_TYPE_STRING)
    {
        char *hal_value = libhal_psi_get_string (iter);
        if (hal_value != NULL)
        {
            value[0] = strdup(hal_value);
        }
    }
    else if (type == LIBHAL_PROPERTY_TYPE_BOOLEAN)
    {
        dbus_bool_t value_b = libhal_psi_get_bool (iter);
        if (value_b == TRUE)
        {
            value[0] = strdup("true");
        }
        else
        {
            value[0] = strdup("false");
        }
    }
    else if (type == LIBHAL_PROPERTY_TYPE_INT32)
    {
        dbus_int32_t int_value = libhal_psi_get_int (iter);
        snprintf(tmp, 255, "%d", int_value);
        tmp[255] = '\0';
        value[0] = strdup(tmp);
    }
    else if (type == LIBHAL_PROPERTY_TYPE_UINT64)
    {
        dbus_uint64_t uint_value = libhal_psi_get_uint64 (iter);
        snprintf(tmp, 255, "%llu", uint_value);
        tmp[255] = '\0';
        value[0] = strdup(tmp);
    }
    else if (type == LIBHAL_PROPERTY_TYPE_DOUBLE)
    {
        double dble_value = libhal_psi_get_double (iter);
        snprintf(tmp, 255, "%g", dble_value);
        tmp[255] = '\0';
        value[0] = strdup(tmp);
    }
    else
    {
        DEBUG(_("Unhandled HAL iterator type: %d"), type);
    }
    return value;
}

char **halevt_get_property_value(LibHalPropertyType type,
  const char *property, const char *udi, DBusError *dbus_error_pointer)
{
    char **value = NULL;
    char tmp[256];

    if (type == LIBHAL_PROPERTY_TYPE_STRLIST)
    {
       char **str_list = libhal_device_get_property_strlist
          (hal_ctx, udi, property, dbus_error_pointer);
       char **new_list = halevt_duplicate_str_list(str_list);
       libhal_free_string_array(str_list);
       return new_list;
    }

    value = malloc(2*sizeof(char *));
    if (value == NULL) { return NULL; };
    value[1] = NULL;

    if (type == LIBHAL_PROPERTY_TYPE_STRING)
    {
        char *hal_value = libhal_device_get_property_string
            (hal_ctx, udi, property, dbus_error_pointer);
        if (hal_value != NULL)
        {
            value[0] = strdup(hal_value);
            libhal_free_string(hal_value);
        }
    }
    else if (type == LIBHAL_PROPERTY_TYPE_BOOLEAN)
    {
        dbus_bool_t value_b = libhal_device_get_property_bool
            (hal_ctx, udi, property, dbus_error_pointer);
        if (value_b == TRUE)
        {
            value[0] = strdup("true");
        }
        else
        {
            value[0] = strdup("false");
        }
    }
    else if (type == LIBHAL_PROPERTY_TYPE_INT32)
    {
        dbus_int32_t int_value = libhal_device_get_property_int
               (hal_ctx, udi, property, dbus_error_pointer);
        snprintf(tmp, 255, "%d", int_value);
        tmp[255] = '\0';
        value[0] = strdup(tmp);
    }
    else if (type == LIBHAL_PROPERTY_TYPE_UINT64)
    {
        dbus_uint64_t uint_value = libhal_device_get_property_uint64
                (hal_ctx, udi, property, dbus_error_pointer);
        snprintf(tmp, 255, "%llu", uint_value);
        tmp[255] = '\0';
        value[0] = strdup(tmp);
    }
    else if (type == LIBHAL_PROPERTY_TYPE_DOUBLE)
    {
        double dble_value = libhal_device_get_property_double
                (hal_ctx, udi, property, dbus_error_pointer);
        snprintf(tmp, 255, "%g", dble_value);
        tmp[255] = '\0';
        value[0] = strdup(tmp);
    }
    else
    {
        DEBUG(_("Unhandled HAL type for property %s, device %s: %d"), property, udi, type);
    }
    return value;
}

/*
 * run the command specified in halevt_exec, performing substitution
 * of hal properties.
 */
int halevt_run_command(const halevt_exec *exec, char const *udi,
   const halevt_device *device)
{
    char *argv[4];
    int string_size = 100;
    int current_size = 1;
    char *command;
    GError *error = NULL;
    int string_index = 0;
    char *string;
    char **values;
    int str_len;
    int hal_value_used;

    if ((command = (char *) malloc (string_size*sizeof(char))) == NULL)
    {
       DEBUG(_("Out of memory, cannot run %s"), exec->string);
    }
    *command = '\0';

    while(exec->elements[string_index].string != NULL)
    {
        hal_value_used = 0;
        if (exec->elements[string_index].hal_property)
        {
            values = halevt_property_value (exec->elements[string_index].string, udi, device);
            if (values == NULL)
            {
                DEBUG(_("Hal property %s in command '%s' not found (or OOM)"), exec->elements[string_index].string, exec->string);
                string = "UNKNOWN";
            }
            else
            {
                string = values[0];
                hal_value_used = 1;
            }
        }
        else
        {
             string = exec->elements[string_index].string;
        }
        str_len = strlen(string);
        if (str_len + string_size > current_size)
        {
            string_size = 2 *(str_len + string_size);
            current_size += str_len;
            if ((command = (char *) realloc (command, string_size*sizeof(char))) == NULL)
            {
                if (hal_value_used) { FREE_NULL_ARRAY(char *, values, free); }
                DEBUG(_("Out of memory, cannot run %s"), exec->string);
                return 0;
            }
            strcat (command, string);
            if (hal_value_used) { free (string); }
        }
        string_index++;
    }

    argv[0] = "/bin/sh";
    argv[1] = "-c";
    argv[2] = command;
    argv[3] = NULL;

    DEBUG(_("Running: %s"), argv[2]);

    if (!g_spawn_async(NULL, argv, NULL, 0, NULL, NULL, NULL, &error))
    {
        DEBUG(_("Execution of '%s' failed with error: %s"),
              command, error->message);
        free(command);
        return 0;
    }

    free(command);
    return 1;
}
