/*
    debug.c output function
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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "common.h"

int halevt_priority = LOG_INFO;

void debug(const char *format, const char *file, const int line,
           const char *func, ...)
{
   va_list ap;
   char *buffer, *buffer_next;
   int n, size = 100;
   
   if (halevt_fork)
   {
      va_start (ap, func);
      openlog (PACKAGE_NAME, 0, LOG_USER);
      vsyslog (halevt_priority, format, ap);
      closelog ();
      va_end (ap);
      return;
   }
   buffer = (char *) malloc(size * sizeof(char));
   if (buffer == NULL) { return; }
/* from vsnprinf man page */
   while (1) {
   /* Try to print in the allocated space. */
      va_start(ap, func);
      n = vsnprintf (buffer, size, format, ap);
      va_end(ap);
      /* If that worked, return the string. */
      if (n > -1 && n < size) { break; }
      /* Else try again with more space. */
      if (n > -1)    /* glibc 2.1 */
          size = n+1; /* precisely what is needed */
      else           /* glibc 2.0 */
          size *= 2;  /* twice the old size */
      if ((buffer_next = realloc (buffer, size)) == NULL) 
      {
         free(buffer);
         return;
      } 
      else 
      {
          buffer = buffer_next;
      }
   }
   size = strlen(buffer) + strlen(file) + strlen(func) + 26;
   buffer_next = malloc(sizeof(char *)* size);
   if (buffer_next == NULL)
   {
      free(buffer);
      return;
   }
   if (halevt_report)
   {
      snprintf(buffer_next, size - 1, "%s", buffer);
      fprintf (stdout, "%s\n", buffer_next);
   }
   else
   {
      snprintf(buffer_next, size - 1, "%s:%d (%s) %s", 
         file, line, func, buffer);
      buffer_next[size-1] = '\0';
      fprintf (stderr, "%s\n", buffer_next);
   }
   free(buffer);
   free(buffer_next);
}
