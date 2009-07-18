/*
    halevt-mount.c use HAL to mount, umount and keep a record of 
    mounted devices
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


#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <libintl.h>
#include <locale.h>

#include <dbus/dbus.h>
#include <libhal.h>

#define _(str) gettext(str)

#define SYSTEMDIR LOCALSTATEDIR "/lib/" PACKAGE 
#define DIRHOME "/." PACKAGE "-mount"
#define LOCKFILENAME "/" PACKAGE ".lock"
#define DEVICEFILENAME "/uditab"
#define TMPPOSTFIX ".tmp"
#define UMASKOPTSTR "umask="
#define UIDOPTSTR "uid="

typedef struct halevt_mount_udi
{
  char *udi;
  char *device;
  char *mountpoint;
  int ignore;
  struct halevt_mount_udi *next;
} halevt_mount_udi;

static halevt_mount_udi *udi_device_mountpoint_root = NULL;

static int halevt_mount_parse_file(const char *file)
{
  FILE *fd = fopen(file, "r");
  char line[1024];
  char *next;
  char *current;
  halevt_mount_udi *new_udi;
  int line_nr = 0;

  if (fd == NULL)
  {
    fprintf(stderr, _("Error opening %s: %s\n"), file, strerror(errno));
    return 0;
  }
  while ((fgets(line, 1023, fd)) != NULL)
  {
     line_nr++;
     if ((new_udi = malloc(sizeof(halevt_mount_udi))) == NULL) { return 0; }

     if (strlen (line) < 1) { 
        fprintf (stderr, _("Empty line\n"));
        continue ;
     }
     if (line[strlen (line) - 1] == '\n') { line[strlen (line) - 1] = '\0';}
     new_udi->device = NULL;
     new_udi->mountpoint = NULL;

     if ((next = strstr (line, ":")) == NULL)
     {
        new_udi->udi = strdup (line);
        if (new_udi->udi == NULL) { return 0; }
     }
     else 
     {
        if (strlen (next) != 1)
        {
           *next = '\0';
           current = next+1;
           if ((next = strstr (current, ":")) == NULL)
           {
              new_udi->device = strdup (current);
              if (new_udi->device == NULL) { return 0; }
           }
           else
           {
              if (strlen (next) != 1)
              {
                 *next = '\0';
                 new_udi->mountpoint = strdup (next+1);
                 if (new_udi->mountpoint == NULL) { return 0; }
              }
              else
              {
                 *next = '\0';
              }
              new_udi->device = strdup (current);
              if (new_udi->device == NULL) { return 0; }
           }
        }
        else
        {
           *next = '\0';
        }
        new_udi->udi = strdup(line);
        if (new_udi->udi == NULL) { return 0; }
     }
     if (strlen (new_udi->udi) == 0) 
     { 
        fprintf(stderr, _("Empty udi: line %d\n"), line_nr);
        continue; 
     }
     
     if ((new_udi->device != NULL) && strlen (new_udi->device) == 0) 
     { 
        fprintf(stderr, _("Empty device: line %d\n"), line_nr);
        new_udi->device = NULL;
     }

     if ((new_udi->mountpoint != NULL) && strlen (new_udi->mountpoint) == 0) 
     { 
        fprintf(stderr, _("Empty mountpoint: line %d\n"), line_nr);
        new_udi->mountpoint = NULL;
     }
     new_udi->next = udi_device_mountpoint_root;
     udi_device_mountpoint_root = new_udi;
/*
     fprintf(stderr,"%s:%s:%s\n", new_udi->udi, new_udi->device, new_udi->mountpoint);
*/
  }
  return 1;
}

void halevt_mount_check_dbus_error(DBusError *error)
{
  if (dbus_error_is_set(error))
  {
    fprintf (stderr, _("DBus Error %s: %s\n"), error->name, error->message);
    dbus_error_free(error);
  }
}

int halevt_mount_check_mount_option(halevt_mount_udi *checked_udi, LibHalContext *hal_ctx, char *mount_option)
{
  DBusError dbus_error;
  char **valid_options; 
  char **current_option; 

  if (checked_udi->udi == NULL) 
  { 
    fprintf (stderr, "BUG: NULL udi in halevt_mount_check_mount_option\n");
    exit (1);
  }

  dbus_error_init(&dbus_error);

  if (! libhal_device_exists(hal_ctx, checked_udi->udi, &dbus_error))
  {
    /* fprintf (stderr, "udi %s seems to have vanished\n", checked_udi->udi);*/
    halevt_mount_check_dbus_error (&dbus_error);
    return 0;
  }
  halevt_mount_check_dbus_error (&dbus_error);

  if (! libhal_device_property_exists (hal_ctx, checked_udi->udi, "volume.mount.valid_options", &dbus_error))
  {
    halevt_mount_check_dbus_error (&dbus_error);
    return 0;
  }
  
  valid_options = libhal_device_get_property_strlist (hal_ctx, checked_udi->udi, "volume.mount.valid_options", &dbus_error);
  if (valid_options == NULL)
  {
    halevt_mount_check_dbus_error (&dbus_error);
    return 0;
  }
  halevt_mount_check_dbus_error (&dbus_error);
  current_option = valid_options;
  while ((*current_option) != NULL)
  {
    if (! strcmp ((*current_option), mount_option))
       return 1;
    current_option++;
  }
  return 0;
}
 
int halevt_mount_check_udi(halevt_mount_udi *checked_udi, LibHalContext *hal_ctx, int warn)
{
  DBusError dbus_error;

  char *hal_device;
  char *hal_mountpoint;
  char *hal_fsusage;

  if (checked_udi->udi == NULL) 
  { 
    fprintf (stderr, "BUG: NULL udi in halevt_mount_check_udi\n");
    exit (1);
  }

  dbus_error_init(&dbus_error);

  if (! libhal_device_exists(hal_ctx, checked_udi->udi, &dbus_error))
  {
    /* fprintf (stderr, "udi %s seems to have vanished\n", checked_udi->udi);*/
    halevt_mount_check_dbus_error (&dbus_error);
    return 0;
  }
  halevt_mount_check_dbus_error (&dbus_error);

  if (! libhal_device_property_exists (hal_ctx, checked_udi->udi, "block.is_volume", &dbus_error))
  {
    if (warn)
      fprintf (stderr, _("udi %s don't seem to be a volume\n"), checked_udi->udi);
    halevt_mount_check_dbus_error (&dbus_error);
    return 0;
  }

  halevt_mount_check_dbus_error (&dbus_error);

  if (! libhal_device_get_property_bool (hal_ctx, checked_udi->udi, "block.is_volume", &dbus_error))
  {
    if (warn)
      fprintf (stderr, _("udi %s isn't a volume\n"), checked_udi->udi);
    halevt_mount_check_dbus_error (&dbus_error);
    return 0;
  }

  halevt_mount_check_dbus_error (&dbus_error);

  
  if (! libhal_device_property_exists (hal_ctx, checked_udi->udi, "block.device", &dbus_error))
  {
    if (warn)
      fprintf (stderr, _("udi %s don't seem to be a block device\n"), checked_udi->udi);
    halevt_mount_check_dbus_error (&dbus_error);
    return 0;
  }
  halevt_mount_check_dbus_error (&dbus_error);

  hal_device = libhal_device_get_property_string (hal_ctx, checked_udi->udi, "block.device", &dbus_error);
  halevt_mount_check_dbus_error (&dbus_error);
  if (hal_device == NULL)
  {
    fprintf (stderr, _("Certainly out of memory, aborting\n"));
    exit (1);
  }
  if (checked_udi->device == NULL)
  {
    if ((checked_udi->device = strdup (hal_device)) == NULL)
    {
      fprintf (stderr, _("Out of memory\n"));
      exit (1);
    }
  }
  else if (strcmp (hal_device, checked_udi->device))
  {
    fprintf (stderr, _("for udi %s, device in file %s isn't the device in hal: %s\n"), checked_udi->udi, checked_udi->device, hal_device);
    if ((checked_udi->device = strdup (hal_device)) == NULL)
    {
      fprintf (stderr, "Out of memory\n");
      exit (1);
    }
  }

  if (! libhal_device_property_exists (hal_ctx, checked_udi->udi, "volume.fsusage", &dbus_error))
  {
    fprintf (stderr, _("udi %s don't seem to be a volume\n"), checked_udi->udi);
    halevt_mount_check_dbus_error (&dbus_error);
    return 0;
  }

  halevt_mount_check_dbus_error (&dbus_error);

  hal_fsusage = libhal_device_get_property_string (hal_ctx, checked_udi->udi, "volume.fsusage", &dbus_error);
  halevt_mount_check_dbus_error (&dbus_error);
  if (hal_fsusage == NULL)
  {
    fprintf (stderr, _("Certainly out of memory, aborting\n"));
    exit (1);
  }

  if (strcmp (hal_fsusage, "filesystem"))
  {
    if (warn)
      fprintf (stderr, _("udi %s does not seem to contain a filesystem\n"), checked_udi->udi);
    return 0;
  }
  
  if (libhal_device_property_exists (hal_ctx, checked_udi->udi, "volume.ignore", &dbus_error))
  {
    halevt_mount_check_dbus_error (&dbus_error);
    if (libhal_device_get_property_bool (hal_ctx, checked_udi->udi, "volume.ignore", &dbus_error))
    {
      checked_udi->ignore = 1;
    }
    else
    {
      checked_udi->ignore = 2;
    }
    halevt_mount_check_dbus_error (&dbus_error);
  }
  halevt_mount_check_dbus_error (&dbus_error);

  if (! libhal_device_property_exists (hal_ctx, checked_udi->udi, "volume.is_mounted", &dbus_error))
  {
    if (warn)
      fprintf (stderr, _("udi %s don't have information about volume mounting\n"), checked_udi->udi);
    halevt_mount_check_dbus_error (&dbus_error);
    return 0;
  }

  halevt_mount_check_dbus_error (&dbus_error);

  if (libhal_device_get_property_bool (hal_ctx, checked_udi->udi, "volume.is_mounted", &dbus_error))
  {

    halevt_mount_check_dbus_error (&dbus_error);
  
    if (! libhal_device_property_exists (hal_ctx, checked_udi->udi, "volume.mount_point", &dbus_error))
    {
      fprintf (stderr, _("udi %s don't have the mountpoint property defined\n"), checked_udi->udi);
      halevt_mount_check_dbus_error (&dbus_error);
      return 0;
    }
    halevt_mount_check_dbus_error (&dbus_error);

    hal_mountpoint = libhal_device_get_property_string (hal_ctx, checked_udi->udi, "volume.mount_point", &dbus_error);
    halevt_mount_check_dbus_error (&dbus_error);
    if (hal_mountpoint == NULL)
    {
      fprintf (stderr, _("Certainly out of memory, aborting\n"));
      exit (1);
    }
    if (checked_udi->mountpoint == NULL)
    {
      if ((checked_udi->mountpoint = strdup (hal_mountpoint)) == NULL)
      {
        fprintf (stderr, _("Out of memory\n"));
        exit (1);
      }
    }
    else if (strcmp (hal_mountpoint, checked_udi->mountpoint))
    {
      fprintf (stderr, _("for udi %s, mountpoint in file %s isn't the mountpoint in hal: %s\n"), checked_udi->udi, checked_udi->mountpoint, hal_mountpoint);
      if ((checked_udi->mountpoint = strdup (hal_mountpoint)) == NULL)
      {
        fprintf (stderr, _("Out of memory\n"));
        exit (1);
      }
    }
  }
  else
  {

    halevt_mount_check_dbus_error (&dbus_error);
    if (checked_udi->mountpoint != NULL)
    {
    /*
      fprintf (stderr, "For udi %s, mountpoint set to %s but it is not mounted\n", checked_udi->udi, checked_udi->mountpoint);
     */
      checked_udi->mountpoint = NULL;
    }
  }
  return 1;
}

void halevt_mount_remove_udi (halevt_mount_udi *current_udi, 
     halevt_mount_udi **previous_udi)
{
  if ((*previous_udi) == udi_device_mountpoint_root)
  {
    udi_device_mountpoint_root = current_udi->next;
    (*previous_udi) = udi_device_mountpoint_root;
  }
  else
  {
    (*previous_udi)->next = current_udi->next;
  }
}

void halevt_mount_sync_udi (const char *file, LibHalContext *hal_ctx)
{
  halevt_mount_udi *current_udi;
  halevt_mount_udi *previous_udi;
  struct stat statbuf;

  if (stat (file, &statbuf) == 0)
  {
    if (! halevt_mount_parse_file (file)) 
    { 
      fprintf (stderr, "Error parsing %s\n", file); 
      exit (1); 
    }

    current_udi = udi_device_mountpoint_root;
    previous_udi = current_udi;
    while (current_udi != NULL)
    {
      if (! halevt_mount_check_udi (current_udi, hal_ctx, 1))
      {
        halevt_mount_remove_udi (current_udi, &previous_udi);
      }
      else
      {
         previous_udi = current_udi;
      }
      current_udi = current_udi->next;
    }
  }
}

int halevt_mount_print_udis (FILE *file, int print_ignore)
{
  halevt_mount_udi *current_udi = udi_device_mountpoint_root;
  char *ignore_string = "";

  while (current_udi != NULL)
  {
    if (print_ignore)
    {
      if (current_udi->ignore == 1)
      {
        ignore_string = _(" (ignore true)");
      }
      else if (current_udi->ignore == 2)
      {
        ignore_string = _(" (ignore false)");
      }
    }

    if (current_udi->mountpoint != NULL && current_udi->device != NULL)
    {
      if (fprintf (file, "%s:%s:%s%s\n", current_udi->udi, current_udi->device, current_udi->mountpoint, ignore_string) < 0)
      {
        return 0;
      }
    }
    else if (current_udi->mountpoint != NULL)
    {
      fprintf (stderr, "Warning: mountpoint without device\n");
      if (fprintf (file, "%s::%s%s\n", current_udi->udi, current_udi->mountpoint, ignore_string) < 0)
      {
        return 0;
      }
    }
    else if (current_udi->device != NULL)
    {
      if (fprintf (file, "%s:%s%s\n", current_udi->udi, current_udi->device, ignore_string) < 0)
      {
        return 0;
      }
    }
    else
    {
      if (fprintf (file, "%s%s\n", current_udi->udi, ignore_string) < 0)
      {
        return 0;
      }
    }
    current_udi = current_udi->next;
  }
  return 1;
}

halevt_mount_udi **halevt_mount_find_udi (const char *udi, const char *device,
 const char *mountpoint, const char *device_mountpoint_udi)
{
  halevt_mount_udi *current_udi = udi_device_mountpoint_root;
  halevt_mount_udi **list;
  int list_size = 2;
  int list_nr = 0;

  if ((list = (halevt_mount_udi **) malloc (list_size * sizeof(halevt_mount_udi *))) == NULL)
  {
    fprintf (stderr, _("Out of memory\n"));
    exit (1);
  }
  list[0] = NULL;

  while (current_udi != NULL)
  {
    if (((udi != NULL) && (! strcmp (current_udi->udi, udi))) || 
      ((device != NULL) && (current_udi->device != NULL) && (! strcmp (current_udi->device, device))) ||
      ((mountpoint != NULL) && (current_udi->mountpoint != NULL) && (! strcmp (current_udi->mountpoint, mountpoint))) ||
      ((device_mountpoint_udi != NULL) && (
         (! strcmp (current_udi->udi, device_mountpoint_udi)) || 
         ((current_udi->device != NULL) && (! strcmp(current_udi->device, device_mountpoint_udi))) ||
         ((current_udi->mountpoint != NULL) && (! strcmp(current_udi->mountpoint, device_mountpoint_udi))))))
    { 
      if ((device != NULL) && (current_udi->device != NULL) && (strcmp (current_udi->device, device)))
      {
        fprintf (stderr, _("device found %s and device specified %s don't match\n"), current_udi->device, device);
      }
      if ((mountpoint != NULL) && (current_udi->mountpoint != NULL) && (strcmp (current_udi->mountpoint, mountpoint)))
      {
        fprintf (stderr, _("mountpoint found %s and mountpoint specified %s don't match\n"), current_udi->mountpoint, mountpoint);
      }
      /* 1 for the new element, 1 for the NULL pointer at the end */
      if ((list_nr + 2) > list_size)
      {
        list_size = 2*list_size;
        if ((list = (halevt_mount_udi **) realloc (list, list_size * sizeof(halevt_mount_udi *))) == NULL)
        {
          fprintf (stderr, _("Out of memory\n"));
          exit (1);
        }
      }
      list[list_nr] = current_udi;
      list_nr++;
    }
    current_udi = current_udi->next;
  }
  list[list_nr] = NULL;
  return list;
}

void halevt_mount_umount (DBusConnection *dbus_connection, halevt_mount_udi *udi)
{
  DBusError dbus_error;
  DBusMessage *dbus_msg;
  DBusMessage *dbus_reply;
  char **options_array;
  
  if ((dbus_msg = dbus_message_new_method_call ("org.freedesktop.Hal", udi->udi,
       "org.freedesktop.Hal.Device.Volume", "Unmount")) == NULL)
  {
    fprintf (stderr, "Out of memory\n");
    return;
  }
  if (! (dbus_message_append_args (dbus_msg, 
       DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &options_array, 0, 
       DBUS_TYPE_INVALID)))
  {
    fprintf (stderr, _("Out of memory\n"));
    return;
  }
    
  dbus_error_init (&dbus_error);
  if (((dbus_reply = dbus_connection_send_with_reply_and_block 
      (dbus_connection, dbus_msg, -1, &dbus_error)) == NULL) || dbus_error_is_set (&dbus_error))
  {
    fprintf (stderr, _("Unmount error for %s:\n"), udi->udi);
  }
  halevt_mount_check_dbus_error (&dbus_error);
}

halevt_mount_udi *halevt_mount_new_udi (char *udi)
{
  halevt_mount_udi *new_udi = malloc(sizeof(halevt_mount_udi));

  if (new_udi == NULL)
  {
     fprintf (stderr, _("Out of memory\n"));
     exit (1);
  }
  new_udi->udi = udi;
  new_udi->device = NULL;
  new_udi->mountpoint = NULL;
  new_udi->ignore = 0;
  return new_udi;
}

halevt_mount_udi *halevt_mount_add_udi (char *udi, char *device, LibHalContext *hal_ctx)
{
  DBusError dbus_error;
  char **all_devices;
  char **current_device;
  int num_devices;  
  halevt_mount_udi *new_udi;

  /* 
    fprintf (stderr, "Add %s, %s\n", udi, device); 
  */
  if (device != NULL)
  {
    dbus_error_init (&dbus_error);
    all_devices = libhal_get_all_devices (hal_ctx, &num_devices, &dbus_error);
    halevt_mount_check_dbus_error (&dbus_error);
    if (all_devices == NULL)
    {
      fprintf (stderr, _("HAL didn't return any device\n"));
      exit (1); 
    }

    current_device = all_devices;
    while ((*current_device) != NULL)
    {
      new_udi = halevt_mount_new_udi (*current_device);
      if (halevt_mount_check_udi (new_udi, hal_ctx, 0))
      { /* found a suitable device */
        if (strcmp (new_udi->device, device))
        { /* no the same device */
          free (new_udi);
        }
        else
        {
          if ((udi != NULL) && (strcmp (udi, new_udi->udi)))
          {
            fprintf (stderr, _("Device %s is associated with udi %s, but udi %s was specified\n"), device, new_udi->udi, udi);
          }
          new_udi->next = udi_device_mountpoint_root;
          udi_device_mountpoint_root = new_udi;
          return new_udi;
        }
      }
      current_device++;
    }
  }

  if (udi == NULL)
  {
    fprintf (stderr, _("The specified device %s wasn't found\n"), device);
    exit (1);
  }

  new_udi = halevt_mount_new_udi (udi);
  if (! halevt_mount_check_udi (new_udi, hal_ctx, 1))
  {
    fprintf (stderr, _("The specified udi %s doesn't seem to exist\n"), udi);
    exit (1);
  }
  new_udi->next = udi_device_mountpoint_root;
  udi_device_mountpoint_root = new_udi;
  return new_udi;
}

void halevt_mount_rewrite_device_file (const char *tmp_device_file, const char *device_file)
{
  FILE *out_fd;
  struct stat statbuf;
  
  if (udi_device_mountpoint_root == NULL)
  {
    if (stat (device_file,  &statbuf) == 0)
    {
      if (unlink (device_file) < 0)
      {
        fprintf (stderr, _("Cannot unlink %s: %s\n"), device_file, strerror(errno));      
      }
    }
  }
  else 
  {
    if ((out_fd = fopen (tmp_device_file, "w")) == NULL)
    {
      fprintf (stderr, _("Error opening %s: %s\n"), tmp_device_file, strerror (errno));
      exit (1);
    }
    if (! halevt_mount_print_udis (out_fd, 0)) { exit (1); }
    if (rename (tmp_device_file, device_file) <0)
    {
      fprintf (stderr, _("Renaming %s to %s failed: %s\n"), tmp_device_file, device_file, strerror (errno));
    }
  }
}

void usage()
{
  printf (_(
" Usage: halevt-mount [option]\n\
        halevt-umount [option]\n\
 Mount or umount device through hal and keep a track of devices handled.\n\
 When called as halevt-umount the default is to unmount, when called as\n\
 halevt-mount the default is to mount.\n\
 Option -c, -l, -r, -s or -w change the operation performed.\n\
   -a               When listing, list all the existing devices.\n\
                    For the other operations, perform the operation on\n\
                    all the handled devices\n\
   -c               remove the handled device\n\
   -d x             use device <x>\n\
   -f x             use file <x> to store information about the handled\n\
                    devices instead of the default\n\
   -i x             use lock file and device information file in the \n\
                    directory <x> (if they are not already specified)\n\
   -l               list handled devices. Formatted with, on each line:\n\
                                 udi:device:mount point\n\
   -p x             use mountpoint <x>\n\
   -m x             set mount umask to <x> if run as a system user\n\
   -n x             use file <x> as lock file\n\
   -o x             add the option <x> to the mount call\n\
   -r               unmount\n\
   -s               sync the information about the handled devices\n\
                    with the informations known by HAL\n\
   -u x             use udi <x>\n\
   -w               add to the handled devices\n\n\
 Additional argument is considered to be a mount point, a device or an udi\n\
 when unmounting or removing.\n\n\
 If the commands are not called with -s after changes in the state of the\n\
 devices (after mounting, unmounting, removing devices) , the information\n\
 available for halevt-mount and halevt-umount may become out of sync with\n\
 the state of the system. \n\
 "
)
); 
}

int halevt_mount_add_option(char ***options_array, char * option, int option_nr)
{
  option_nr++;
  if ((*options_array = realloc (*options_array, option_nr * sizeof(char *))) == NULL)
  {
    fprintf (stderr, _("Out of memory\n"));
    return 0;
  }
  if (((*options_array)[option_nr - 1] = strdup(option)) == NULL)
  {
    fprintf (stderr, _("Out of memory\n"));
    return 0;
  }
  return option_nr;
}

int main (int argc, char **argv)
{
  halevt_mount_udi *current_udi;
  halevt_mount_udi *previous_udi;
  halevt_mount_udi **udi_found;
  halevt_mount_udi **current_udi_found;
  FILE *out_fd;
  int fd_lock;
  int c;
  int do_all = 0;
  int set_umask = 0;
  char *device_file = NULL;
  char *tmp_device_file;
  char *device = NULL;
  char *udi = NULL;
  char *mountpoint = NULL;
  char *lockfile = NULL;
  char *directory = NULL;
  char *cmd_name;
  char *cmd = NULL;
  char *home;
  int arg_index = 0;
  struct stat statbuf;
  struct passwd *passwd_struct;
  DBusMessage *dbus_msg;
  DBusMessage *dbus_reply;
  DBusError dbus_error;
  DBusConnection *dbus_connection;
  LibHalContext *hal_ctx;
  char *fstype = "";
  char **options_array;
  int option_nr = 0;
  char **all_devices;
  char **current_device;
  halevt_mount_udi *new_device_udi;
  int num_devices;
  int system_user = 0;
  char *mask_string;
  char uid_nr[50];
  char *uid_string;

  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, DATADIR "/locale/");
  textdomain (PACKAGE);

  if ((options_array = (char **) malloc (sizeof(char *))) == NULL)
  {
    fprintf (stderr, _("Out of memory\n"));
    exit (1);
  }

  if ((passwd_struct = getpwuid(getuid())) == NULL)
  {
    fprintf (stderr, _("Error for getpwuid: %s\n"), strerror (errno));
  }
  if ((! strcmp(passwd_struct->pw_name, HALEVT_USER_NAME)) || (getuid() == 0))
  {
    system_user = 1;
  }

  while (1)
  {
     c = getopt(argc, argv, "u:d:m:n:p:f:i:o:alschrw");
     if (c == -1) { break ; }
     switch (c)
     {
       case 'i':
         if ((directory = strdup(optarg)) == NULL)
         {
           fprintf (stderr, _("Out of memory\n"));
           exit (1);
         }
         break;
       case 'f':
         if ((device_file = strdup(optarg)) == NULL)
         {
           fprintf (stderr, _("Out of memory\n"));
           exit (1);
         }
         break;
       case 'u':
         if ((udi = strdup(optarg)) == NULL)
         {
           fprintf (stderr, _("Out of memory\n"));
           exit (1);
         }
         break;
       case 'n':
         if ((lockfile = strdup(optarg)) == NULL)
         {
           fprintf (stderr, _("Out of memory\n"));
           exit (1);
         }
         break;
       case 'd':
         if ((device = strdup(optarg)) == NULL)
         {
           fprintf (stderr, _("Out of memory\n"));
           exit (1);
         }
         break;
       case 'p':
         if ((mountpoint = strdup(optarg)) == NULL)
         {
           fprintf (stderr, _("Out of memory\n"));
           exit (1);
         }
         break;
       case 'o':
         option_nr = halevt_mount_add_option(& options_array, optarg, option_nr);
         if (option_nr == 0) { exit (1); }
         break;
       case 'm':
         if (system_user)
         {
           if ((mask_string = (char *) malloc ((1 + strlen (optarg) + strlen (UMASKOPTSTR)) * sizeof(char))) == NULL)
           {
             fprintf (stderr, _("Out of memory\n"));
             exit (1);
           }
           strcpy (mask_string, UMASKOPTSTR);
           strcat (mask_string, optarg);
           set_umask = 1;
         }
         break;
       case 'a':
         do_all = 1;
         break;
       case 'l':
         cmd = "list";
         break;
       case 's':
         cmd = "sync";
         break;
       case 'c':
         cmd = "clean";
         break;
       case 'r':
         cmd = "umount";
         break;
       case 'w':
         cmd = "add";
         break;
       case 'h':
         usage();
         exit (0);
         break;
       default:
         break;
     }
  }

  if (cmd == NULL)
  {
    if ((cmd_name = strrchr(argv[0], '/')) == NULL)
    {
      cmd_name = argv[0];
    }
    else
    {
      cmd_name++;
    }

    if (! strcmp(cmd_name, "halevt-umount")) 
    {   
      cmd = "umount"; 
    }
    else
    {
      cmd = "mount";
    }
  }
  /* fprintf (stderr, "Executing cmd: %s\n", cmd); */

  if (optind < argc) { arg_index = optind; }
  
  if ((lockfile == NULL || device_file == NULL) && directory == NULL)
  {
    if (system_user)
    {
      directory = SYSTEMDIR;
    }
    else
    {
      if ((home = getenv("HOME")) == NULL)
      {
        fprintf (stderr, _("Cannot getenv HOME\n"));
        exit (1);
      }
      if ((directory = malloc ((1 + strlen (home) + strlen (DIRHOME)) * sizeof(char))) == NULL)
      {
        fprintf (stderr, _("Out of memory\n")); 
        exit(1);
      }
      strcpy (directory, home);
      strcat (directory, DIRHOME);
    }
  }

  if (lockfile == NULL)
  {
    if ((lockfile = malloc ((1 + strlen (directory) + strlen (LOCKFILENAME)) * sizeof(char))) == NULL)
    {
      fprintf (stderr, _("Out of memory\n")); 
      exit(1);
    }
    strcpy (lockfile, directory);
    strcat (lockfile, LOCKFILENAME);
  }

  if (device_file == NULL)
  {
    if ((device_file = malloc ((1 + strlen (directory) + strlen (DEVICEFILENAME)) * sizeof(char))) == NULL)
    {
      fprintf (stderr, _("Out of memory\n")); 
      exit(1);
    }
    strcpy (device_file, directory);
    strcat (device_file, DEVICEFILENAME);
  }

  if ((tmp_device_file = malloc ((1 + strlen (device_file) + strlen(TMPPOSTFIX)) * sizeof(char))) == NULL)
  {
    fprintf (stderr, _("Out of memory\n"));
    exit(1);
  }
  strcpy (tmp_device_file, device_file);
  strcat (tmp_device_file, TMPPOSTFIX);

  if (stat (directory, &statbuf) < 0)
  {
    if (mkdir (directory, 0777) < 0)
    {
      fprintf (stderr, _("Cannot create directory %s: %s\n"), directory, strerror (errno));
      exit (1);
    }
  }
  if ((fd_lock = open (lockfile, O_RDWR | O_CREAT, 0644)) < 0)
  {
    fprintf (stderr, _("Error opening lock file %s: %s\n"), lockfile, strerror (errno));
  }
  if (lockf (fd_lock, F_LOCK, 0) < 0)
  {
    fprintf (stderr, _("Error locking lock file %s: %s\n"), lockfile, strerror (errno));
  }
  
  if (stat (tmp_device_file,  &statbuf) == 0)
  {
    if (unlink (tmp_device_file) < 0)
    {
      fprintf (stderr, _("Cannot unlink %s: %s\n"), tmp_device_file, strerror(errno));      
    }
  }

  if ((! strcmp (cmd, "clean")) && do_all)
  {
    if (stat (device_file,  &statbuf) == 0)
    {
      if (unlink (device_file) < 0)
      {
        fprintf (stderr, _("Cannot unlink %s: %s\n"), device_file, strerror(errno));
      }
    }
    goto end;
  }

  if ((! strcmp (cmd, "list")) && (! do_all))
  {
    if (stat (device_file,  &statbuf) == 0)
    {
      if (! halevt_mount_parse_file (device_file)) { fprintf (stderr, _("Error parsing %s\n"), device_file); }
      else { halevt_mount_print_udis (stdout, 0); }
    }
    goto end;
  }
  /* initialize hal, we are going to synchronize the informations with the
   hal informations */
  if ((hal_ctx = libhal_ctx_new()) == NULL)
  {
    fprintf (stderr, _("Failed to create HAL context\n"));
    exit(1);
  }
  dbus_error_init (&dbus_error);
  dbus_connection = dbus_bus_get (DBUS_BUS_SYSTEM, &dbus_error);
  if (dbus_error_is_set (&dbus_error))
  {
    fprintf (stderr, _("Error connecting to D-BUS system bus: %s"), dbus_error.message);
    dbus_error_free (&dbus_error);
    exit(1);
  }
  libhal_ctx_set_dbus_connection (hal_ctx, dbus_connection);

  if (!libhal_ctx_init (hal_ctx, &dbus_error))
  {
    fprintf (stderr, _("Error initializing HAL"));
    exit(1);
  }
  halevt_mount_sync_udi (device_file, hal_ctx);
  
  if (! strcmp (cmd, "sync"))
  {
    /* write the new udi structure to the temporary file */
    halevt_mount_rewrite_device_file (tmp_device_file, device_file);
  }
  else if (! strcmp (cmd, "list"))
  { /* do_all is true. We show all the devices, including those not handled */
    fprintf (stdout, _("Devices handled:\n"));
    halevt_mount_print_udis (stdout, 0);

    /* get and add all the devices */
    all_devices = libhal_get_all_devices (hal_ctx, &num_devices, &dbus_error);
    halevt_mount_check_dbus_error (&dbus_error);
    if (all_devices == NULL)
    {
      fprintf (stderr, _("HAL didn't return any device\n"));
      exit (1); 
    }
    /* reuse the same udi root list */
    udi_device_mountpoint_root = NULL;
    current_device = all_devices;
    while ((*current_device) != NULL)
    {
      new_device_udi = halevt_mount_new_udi (*current_device);
      if (halevt_mount_check_udi (new_device_udi, hal_ctx, 0))
      {
        new_device_udi->next = udi_device_mountpoint_root;
        udi_device_mountpoint_root = new_device_udi;
      }
      current_device++;
    }
    fprintf (stdout, _("\nAll devices:\n"));
    halevt_mount_print_udis (stdout, 1);
  }
  else if ((! strcmp (cmd, "mount")) || (! strcmp (cmd, "add")))
  {
    if ((udi == NULL) && (device == NULL))
    {
      fprintf (stderr, _("An udi or a device is needed\n"));
      exit (1);
    }
    udi_found = halevt_mount_find_udi (udi, device, NULL, NULL);
    if (udi_found[0] == NULL) 
    { 
       new_device_udi = halevt_mount_add_udi (udi, device, hal_ctx); 
    }
    else { new_device_udi = udi_found[0]; }
    /* write the new udi structure to the temporary file */
    if ((out_fd = fopen (tmp_device_file, "w")) == NULL)
    {
      fprintf (stderr, _("Error opening %s: %s\n"), tmp_device_file, strerror (errno));
      exit (1);
    }
    if (! halevt_mount_print_udis (out_fd, 0)) { exit (1); }

    if (! strcmp (cmd, "add")) 
    { 
    /* if we were just adding a new device we are done now */
      if (rename (tmp_device_file, device_file) < 0)
      {
        fprintf (stderr, _("Renaming %s to %s after add failed: %s\n"), tmp_device_file, device_file, strerror (errno));
        exit (1); 
      }
      goto end; 
    }
    /* now perform the mount */
    if ((dbus_msg = dbus_message_new_method_call ("org.freedesktop.Hal", 
       new_device_udi->udi, "org.freedesktop.Hal.Device.Volume", "Mount")) == NULL)
    {
      fprintf (stderr, _("Out of memory\n"));
      exit (1);
    }

    if (mountpoint == NULL) { mountpoint = ""; }

    if (set_umask)
    {
       if (halevt_mount_check_mount_option(new_device_udi, hal_ctx, UMASKOPTSTR))
       {
          option_nr = halevt_mount_add_option(& options_array, mask_string, option_nr);
          if (option_nr == 0) { exit (1); }
       }
       free (mask_string);
    }

    if (! system_user && halevt_mount_check_mount_option(new_device_udi, hal_ctx, UIDOPTSTR))
    {
       snprintf(uid_nr, 49, "%d", getuid());
       uid_nr[49] = '\0';
       if ((uid_string = (char *) malloc ((1 + strlen (uid_nr) + strlen (UIDOPTSTR)) * sizeof(char))) == NULL)
       {
          fprintf (stderr, _("Out of memory\n"));
          exit (1);
       }
       strcpy (uid_string, UIDOPTSTR);
       strcat (uid_string, uid_nr);
       
       option_nr = halevt_mount_add_option(& options_array, uid_string, option_nr);
       if (option_nr == 0) { exit (1); }
       free (uid_string);
    }
    if (! (dbus_message_append_args (dbus_msg,
       DBUS_TYPE_STRING, &mountpoint, DBUS_TYPE_STRING, &fstype,
       DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &options_array, option_nr,
       DBUS_TYPE_INVALID)))
    {
      fprintf (stderr, "Out of memory\n");
      exit (1);
    }
    
    if (((dbus_reply = dbus_connection_send_with_reply_and_block
        (dbus_connection, dbus_msg, -1, &dbus_error)) == NULL)
          || dbus_error_is_set (&dbus_error))
    {
      fprintf (stderr, _("Mount error for %s:\n"), udi);
      halevt_mount_check_dbus_error (&dbus_error);
      exit (1);
    }
    halevt_mount_check_dbus_error (&dbus_error);
    if (rename (tmp_device_file, device_file) <0)
    {
      fprintf (stderr, _("Renaming %s to %s after a mount failed: %s\n"), tmp_device_file, device_file, strerror (errno));
    }
  }
  else if ((! strcmp (cmd, "umount")) || (! strcmp (cmd, "clean")))
  {
    char *searched_device_mountpoint = NULL;
    if (udi == NULL && device == NULL && mountpoint == NULL && (!arg_index) && (!do_all))
    {
      fprintf (stderr, _("An argument or -a is needed\n"));
      exit (1);
    }

    if (do_all)
    { /* do_all and "clean" has already been handled before */
      current_udi = udi_device_mountpoint_root;
      while (current_udi != NULL)
      {
        halevt_mount_umount (dbus_connection, current_udi);
        current_udi = current_udi->next;
      }
    }
    else 
    {
      if (arg_index) { searched_device_mountpoint = argv[arg_index]; }

      udi_found = halevt_mount_find_udi (udi, device, mountpoint, searched_device_mountpoint);
      if (udi_found[0] != NULL)
      {
        if (! strcmp (cmd, "umount"))
        {
          current_udi_found = udi_found;
          while ((*current_udi_found) != NULL)
          {
            halevt_mount_umount (dbus_connection, *current_udi_found);
            current_udi_found++;
          }
        }
        else if (! strcmp (cmd, "clean"))
        { /* remove the udi, device or mountpoints from the list */
          current_udi = udi_device_mountpoint_root;
          previous_udi = current_udi;
          while (current_udi != NULL)
          {
            int udi_to_be_removed = 0;
            current_udi_found = udi_found;
            while ((*current_udi_found) != NULL)
            {
              if (! strcmp (current_udi->udi, (*current_udi_found)->udi))
              {
                udi_to_be_removed = 1;
                break;
              }
              current_udi_found++;
            }
            if (udi_to_be_removed)
            {
              halevt_mount_remove_udi (current_udi, &previous_udi);
            }
            else
            {
              previous_udi = current_udi;
            }
            current_udi = current_udi->next;
          }
        }
      }
      else 
      {
        if (! strcmp (cmd, "list"))
        {  
          fprintf (stderr, _("Cannot find a device to be listed\n"));
        }
        else
        {
          fprintf (stderr, _("Cannot find a device to be umounted\n"));
        }
      }
    }
    /* when umounting we don't write to the device file, to avoid messing
     with file permissions in case the umount was done as root */
    if (! strcmp (cmd, "clean"))
       halevt_mount_rewrite_device_file (tmp_device_file, device_file);
  }
  /* there is no reason why we should go here */
  else { fprintf (stderr, "Bug: Unknown command %s\n", cmd); }

end:
  if (unlink (lockfile) <0)
  {
  /*  Another process may have already removed the file */
  /*  fprintf (stderr, "Cannot unlink %s: %s\n", lockfile, strerror(errno)); */
  }
  
  if (close (fd_lock) <0)
  {
    fprintf (stderr, _("Error on close %s: %s\n"), lockfile, strerror(errno));
  }

  return 0;
}
