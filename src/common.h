/*
    common.h common declarations
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


#ifndef _COMMON_H
#define _COMMON_H

#include <libintl.h>
#define _(str) gettext(str)

#ifndef _CONFIG_H
# define _CONFIG_H
# include "config.h"
#endif

#define WALK_LINKED_LISTP(previous, entry, first) \
   for (previous = NULL, entry = first; \
        entry != NULL; previous = entry, \
        entry = entry->next)

#define WALK_LINKED_LIST(entry, first) \
   for (entry = first; entry != NULL; entry = entry->next)

#define FREE_LINKED_LIST(type, root, free_function) \
   do { \
      type *current; \
      type *next; \
      for (current = root; current != NULL; current = next) \
      { \
         next = current->next; \
         free_function(current); \
      } \
   } while(0)

#define WALK_NULL_ARRAY(entry, first) \
   for (entry = first; entry && *entry != NULL; entry++)

#define FREE_NULL_ARRAY(type, first, free_function) \
   do { \
      type *current; \
      for (current = first; current && *current != NULL; current++) \
      { \
         free_function(*current); \
      } \
      free(first); \
   } while(0)

#define DEBUG(fmt, arg...) \
	do { \
		debug(fmt, __FILE__, __LINE__, __FUNCTION__, ##arg); \
	} while(0)

#define DEBUG_REPORT(fmt, arg...) \
	do { \
		if (halevt_print_callbacks) { DEBUG(fmt, ##arg); } \
	} while(0)

extern int halevt_fork;
extern int halevt_report;
extern int halevt_print_callbacks;
extern int halevt_priority;


void debug(const char *format, const char *file, const int line,
           const char *func, ...);

#endif
