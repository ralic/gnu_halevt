/*
    match.h declarations for match.c
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


#ifndef _MATCH_H
#define _MATCH_H

#include <boolstuff/c-api.h>
#include "devices_list.h"

typedef struct halevt_match
{
  char *name;
  char *value;
  char **parents;
} halevt_match;

typedef struct halevt_boolean_expression 
{
  boolexpr_t tree;
  char *string;
  char *expression_string;
  halevt_match *matches;
  int matches_size;
} halevt_boolean_expression;

void halevt_free_boolean_expression(halevt_boolean_expression *expr);
halevt_boolean_expression *halevt_new_boolean_expression(char *expression);
char *halevt_print_boolean_expression(halevt_boolean_expression *expr);
int halevt_true_tree(const halevt_boolean_expression *expr, const char* udi,
   const halevt_device *device);

#endif
