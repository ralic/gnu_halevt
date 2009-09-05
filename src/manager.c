/*
    manager.c halevt main program
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

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <libintl.h>
#include <dirent.h>
#include <locale.h>

#include <glib.h>

#include "common.h"
#include "hal_interface.h"
#include "parse_config.h"

#define CONFFILE PACKAGE ".xml"
#define CONFFILEEXTENSION ".xml"
#define DATACONFFILE DATADIR "/" PACKAGE "/" CONFFILE
#define SYSCONFFILE SYSCONFDIR "/" PACKAGE "/" CONFFILE
#define DATACONFDIR DATADIR "/" PACKAGE "/"
#define SYSCONFCONFDIR SYSCONFDIR "/" PACKAGE "/"
#define PIDFILE PIDDIR "/" PACKAGE ".pid"

static GMainLoop *loop;

void halevt_signal_handler(int sig)
{
    DEBUG(_("Got signal %d"), sig);
    g_main_loop_quit (loop);
}

#define DIRECTORY_NR 3

char **halevt_find_conf_files()
{
    char *home;
/*    char *sysconffile = SYSCONFFILE;
    char *datafile = DATACONFFILE;
*/
    struct stat statbuf;
    int conffile_nr = 1;
    char **conffiles = malloc (sizeof(char *) * conffile_nr);
    char **basefiles = malloc (sizeof(char *) * conffile_nr);
    char * directories[DIRECTORY_NR];
    char *homedir = NULL;
    int i;
    DIR *dir;
    struct dirent *next_file;
    char *filename = NULL;
    char *extension;
    char *string_pos;
    char **current_basefile;
    int found;

    if (conffiles == NULL || basefiles == NULL) { goto oom; }
    basefiles[0] = NULL;
    conffiles[0] = NULL;

    home = getenv("HOME");
    if (home != NULL)
    {
        if ((homedir =  (char *) malloc ((1 + strlen("/.") + strlen (home) + strlen (PACKAGE) + strlen ("/")))) == NULL)
        {
            goto oom;
        }
        strcpy (homedir, home);
        strcat (homedir, "/.");
        strcat (homedir, PACKAGE);
        strcat (homedir, "/");
    }
    directories[0] = homedir;
    directories[1] = SYSCONFCONFDIR;
    directories[2] = DATACONFDIR;

    for (i = 0; i < DIRECTORY_NR; i++)
    {
        if (directories[i] == NULL) { continue; }
        if ((dir = opendir (directories[i])) == NULL) { continue; }
        while ((next_file = readdir (dir)) != NULL)
        {
           if ((filename = (char *) malloc (strlen (directories[i]) + strlen (next_file->d_name) + 1)) == NULL)
           {
               closedir(dir);
               goto oom;
           }
           strcpy (filename, directories[i]);
           strcat (filename, next_file->d_name);

           if (stat (filename, &statbuf) == 0)
           {
               if (! S_ISREG(statbuf.st_mode))
               {
                   free(filename);
                   continue;
               }
               /* special case, a file named .xml... */
               if (! strcmp (CONFFILEEXTENSION, next_file->d_name))
               {
                   free(filename);
                   continue;
               }

               /* process a file only if it ends with .xml */
               string_pos = next_file->d_name;
               extension = NULL;
               while ((extension = strstr (string_pos, CONFFILEEXTENSION)) != NULL)
               {
                   if (strlen (extension) == strlen (CONFFILEEXTENSION))
                   {
                       break;
                   }
                   string_pos += strlen (CONFFILEEXTENSION);
               }
               if (extension == NULL)
               {
                   free(filename);
                   continue ;
               }
               found = 0;
               WALK_NULL_ARRAY(current_basefile, basefiles)
               {
                   if (! strcmp (*current_basefile, next_file->d_name))
                   {
                       found = 1;
                       break;
                   }
               }
               if (! found)
               {
                   char **tmp_basefiles = (char **) realloc (basefiles, ++conffile_nr * sizeof(char *));
                   char **tmp_conffiles =  (char **) realloc (conffiles, conffile_nr * sizeof(char *));
                   if (tmp_basefiles == NULL || tmp_conffiles == NULL)
                   {
                       free(tmp_basefiles);
                       free(tmp_conffiles);
                       closedir(dir);
                       goto oom;
                   }
                   basefiles = tmp_basefiles;
                   conffiles = tmp_conffiles;
                   conffiles[conffile_nr - 2] = filename;
                   if ((basefiles[conffile_nr - 2] = strdup (next_file->d_name)) == NULL)
                   {
                       closedir(dir);
                       goto oom;
                   }
                   conffiles[conffile_nr - 1] = NULL;
                   basefiles[conffile_nr - 1] = NULL;
               }
           }
           else
           {
               DEBUG(_("stat failed for %s: %s"), filename, strerror (errno));
           }
       }
       closedir(dir);
    }

    FREE_NULL_ARRAY(char *, basefiles, free);
    free(homedir);

    return conffiles;

oom:
    DEBUG(_("Out of memory"));
    FREE_NULL_ARRAY(char *, conffiles, free);
    FREE_NULL_ARRAY(char *, basefiles, free);
    free(homedir);
    free(filename);
    return NULL;
}

void halevt_clear_pidfile(const char *file)
{
    if (file != NULL)
    {
        DEBUG(_("Deleting lockfile %s"), file);
        if (unlink(file) == -1)
        {
            DEBUG(_("Failed to delete lockfile %s"), file);
        }
    }
}

void usage()
{
    printf(_(" Usage: halevt [option]\n\
 Execute arbitrary commands in response to HAL events\n\n\
   -c x             use config file <x> instead of searching below HOME\n\
                    /etc and /usr/share\n\
   -f               stay in the foreground, don't run as a daemon\n\
   -g x             run as group <x>\n\
   -p x             use pid file <x> instead of the default\n\
                    '-' means not to use any pid file\n\
   -u x             run as user <x>\n\n\
   -i               report events, no configuration read, stay in foreground\n"
));
}

int halevt_fork = 0;
int halevt_report = 0;

/**
 * main function.
 */
int main(int argc, char *argv[])
{
    char **conffiles = NULL;
    char **current_conffile;
    const char *user = NULL;
    const char *group = NULL;
    const char *pid_file = NULL;
    struct group *group_struct;
    struct passwd *passwd_struct;
    struct stat statbuf;
    uid_t uid;
    gid_t gid;
    int c;
    int do_fork = 1;

    setlocale (LC_ALL, "");
    bindtextdomain (PACKAGE, DATADIR "/locale/");
    textdomain (PACKAGE);

    while (1)
    {
        c = getopt(argc, argv, "ic:u:g:fp:h");
        if (c == -1) { break ; }
        switch (c)
        {
            case 'c':
               if ((conffiles = (char **) malloc (sizeof(char *) * 2)) == NULL)
               {
                   DEBUG(_("Out of memory"));
                   exit (1);
               }
               if ((conffiles[0] = strdup(optarg)) == NULL)
               {
                   DEBUG(_("Out of memory"));
                   exit (1);
               }
               if  (stat (conffiles[0], &statbuf) < 0)
               {
                   DEBUG(_("stat on %s failed: %s"), conffiles[0], strerror (errno));
                   exit (1);
               }
               conffiles[1] = NULL;
               break;
            case 'i':
               halevt_report = 1;
               do_fork = 0;
               break;
            case 'u':
               user = optarg;
               break;
            case 'g':
               group = optarg;
               break;
            case 'p':
               pid_file = optarg;
               break;
            case 'f':
               do_fork = 0;
               break;
            case 'h':
               usage();
               exit (0);
               break;
            default:
               break;
        }
    }
    /* change id */
    if (geteuid() == 0)
    {
        if (user == NULL) { user = USER; }
        if (group == NULL) { group = GROUP; }
        if (pid_file == NULL) { pid_file = PIDFILE; }
        if (!strcmp(pid_file, "-")) { pid_file = NULL; }

        errno = 0;
        if ((passwd_struct = getpwnam(user)) == NULL)
        {
            DEBUG(_("Error getting uid for %s: %s"), user, strerror(errno));
            exit(1);
        }
        uid = passwd_struct->pw_uid;
        errno = 0;
        if ((group_struct = getgrnam(group)) == NULL)
        {
            DEBUG(_("Error getting gid for %s: %s"), group, strerror(errno));
            exit(1);
        }
        gid = group_struct->gr_gid;
        if (setgid(gid) != 0)
        {
            DEBUG(_("Error setting gid to %u: %s"), gid, strerror(errno));
            exit(1);
        }
        if (setuid(uid) != 0)
        {
            DEBUG(_("Error setting uid to %u: %s"), uid, strerror(errno));
            exit(1);
        }
    }

    if (halevt_report == 0)
    {
       /* parse conf file */
        if (conffiles == NULL) { conffiles = halevt_find_conf_files (); }
        if (conffiles == NULL || conffiles[0] == NULL)
        {
            DEBUG(_("No configuration file found"));
            exit (1);
        }
        WALK_NULL_ARRAY(current_conffile, conffiles)
        {
            if (! halevt_parse_config (*current_conffile))
            {
                DEBUG(_("Configuration file %s parsing failed"), *current_conffile);
                exit(1);
            }
            if (halevt_fork)
            {
                DEBUG(_("Using configuration file %s"), *current_conffile);
            }
            free (*current_conffile);
        }
        free (conffiles);
    }

    /* halevt_print_config(); */
    halevt_setup_HAL();
    
    if (do_fork)
    {
        if (daemon(0, 0) !=0)
        {
            DEBUG(_("Daemonize error: %s"), strerror(errno));
            exit (1);
        }

        halevt_fork = 1;

        DEBUG("%s, http://www.environnement.ens.fr/perso/dumas/halevt.html", PACKAGE_STRING);

        if (pid_file == NULL)
        {
            DEBUG(_("No pid file used"));
        }
        else
        {
            FILE *file = fopen (pid_file, "w");
            if (file == NULL)
            {
                DEBUG(_("Open pid file %s failed: %s"), pid_file, strerror(errno));
                exit (1);
            }
            if (lockf(fileno(file), F_TLOCK, 0) != 0)
            {
                DEBUG(_("Lock %s failed: %s"), pid_file, strerror(errno));
                exit (1);
            }
            if ((fprintf (file, "%d\n", getpid())) < 0)
            {
                DEBUG(_("Writing pid to %s failed"), pid_file);
                exit (1);
            }
            fclose(file);
        }
    }

    halevt_run_oninit ();

    loop = g_main_loop_new(NULL, FALSE);
    if (!loop)
    {
        DEBUG(_("Error creating main loop"));
        return 1;
    }

    signal(SIGTERM, halevt_signal_handler);
    signal(SIGINT, halevt_signal_handler);
    signal(SIGHUP, halevt_signal_handler);

/*
    DEBUG(_("Entering main loop."));
*/
    g_main_loop_run(loop);
/*
    DEBUG(_("Exiting normally."));
*/

    if (halevt_fork) { halevt_clear_pidfile(pid_file); }

    halevt_free_config();
    halevt_free_devices();

    return 0;
}
