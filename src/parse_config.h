/*
    parse_config.h declarations for parse_config.c
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

#ifndef PARSE_CONFIG_H
#define PARSE_CONFIG_H

#include "match.h"

typedef struct halevt_exec_element
{
   char *string;
   int hal_property;
} halevt_exec_element;

typedef struct halevt_exec
{
   char *string;
   char *parsed_string; 
   halevt_exec_element *elements;
} halevt_exec;

typedef struct halevt_property_action
{
   char *value;
   halevt_exec *exec;
   struct halevt_property_action *next;
} halevt_property_action;

typedef struct halevt_property
{
   char *name;
   halevt_boolean_expression *match;
   struct halevt_property_action *action;
   struct halevt_property *next;
} halevt_property;

typedef struct halevt_insertion
{
   halevt_exec *exec;
   halevt_boolean_expression *match;
   struct halevt_insertion* next;
} halevt_insertion;

typedef struct halevt_removal
{
   halevt_exec *exec;
   halevt_boolean_expression *match;
   struct halevt_removal* next;
} halevt_removal;

typedef struct halevt_oninit
{
   halevt_exec *exec;
   halevt_boolean_expression *match;
   struct halevt_oninit* next;
} halevt_oninit;

typedef struct halevt_condition
{
   char *name;
   halevt_exec *exec;
   halevt_boolean_expression *match;
   struct halevt_condition* next;
} halevt_condition;

int halevt_parse_config(char const *path);
char *halevt_hal_string (char *string);
void halevt_print_config ();

halevt_property *halevt_property_root;
halevt_insertion *halevt_insertion_root;
halevt_removal *halevt_removal_root;
halevt_condition *halevt_condition_root;
halevt_oninit *halevt_oninit_root;
#endif
