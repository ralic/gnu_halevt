/*
    match.c parse device matching strings, and check that they are true
    Copyright (C) 2007, 2008  Patrice Dumas <pertusus at free dot fr>

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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libintl.h>

#include "common.h"
#include "match.h"
#include "hal_interface.h"
#include "parse_config.h"

/* transform a string with boolean expression containing any character
 * to a string acceptable by boolstuff, containing only alphanumeric
 * characters. Here, all the atoms are substitued by a numeric index
 * and the atom strings are stored at that index.
 * This function also parses the 'atoms' into a key and a value and
 * does the initial processing of the boolean expression */
halevt_boolean_expression *halevt_new_boolean_expression(const xmlChar *original_string)
{
   halevt_boolean_expression *new_expression;
   char *expr;
   char **atoms;
   int i, s;
   enum bool_error_t bool_error;
   new_expression = malloc (sizeof(halevt_boolean_expression));
   if (new_expression != NULL)
   {
      int expr_string_size = 10;
      int index = 0;
      int atom_sizes = 4;
      char next_char;
      char *atom = NULL;
      /* FIXME this should be set to the length in char of the highest 
         integer that doesn't trigger an integer overflow. For example with 
         32 bit integers the number of integers is 2^32 = 4294967296 
         therefore they may be represented with 10 chars
      */
      int atom_index_max_size = 256;
      char atom_index_string[atom_index_max_size];
      char next_char_string[2];

      new_expression->matches_size = 0;
      /* FIXME xmlChar conversion */
      if ((new_expression->string = strdup((char *)original_string)) == NULL)
      {
         free(new_expression);
         return NULL;
      }

      if ((expr = malloc (expr_string_size*sizeof(char))) == NULL)
      {
         free(new_expression->string);
         free(new_expression);
         return NULL;
      }
      *expr = '\0';
      if ((atoms = (char **) malloc (atom_sizes*sizeof(char *))) == NULL)
      {
         free (new_expression->string);
         free (expr);
         free (new_expression);
         return NULL;
      }
      while (1)
      {
         next_char = *(new_expression->string+index);
         if (next_char == '&' || next_char == '|' ||
             next_char == '(' || next_char == ')' || next_char == '!' ||
             next_char == '\0')
         {
            int atom_index_string_size = 0;

            if (atom != NULL)
            { /* this is the end of an atomic boolean expression */
               int n;
               int b = 0;
               /* add an end of string and remove trailing spaces */
               while (isspace(*(new_expression->string+index-b -1))){ b++; }
               *(new_expression->string+index - b) = '\0';
               /* add the atom to the list of atoms */
               /* FIXME integer overflow? */
               (new_expression->matches_size)++;
               if (new_expression->matches_size >= atom_sizes -1)
               {
                  /* fprintf(stderr, "Enlarging size at %d for %d\n", atom_sizes, new_expression->matches_size); */
                  atom_sizes = atom_sizes*2;
                  atoms = (char **) realloc(atoms, atom_sizes*sizeof(char *));
                  if (atoms == NULL) { goto free_memory; }
               }
               atoms[new_expression->matches_size - 1] = atom;

               /* in the boolean expression, the atom is substituted by
                 the index of the atom */

               /* the atom expression index is put into
                * atom_index_string */
               n = snprintf(atom_index_string, atom_index_max_size, "%d", new_expression->matches_size - 1);
               if (n == -1 || n >= atom_index_max_size)
               { /* the index number representation in char is more
                    than atom_index_max_size characters... This should
                    never happen */
                  DEBUG(_("Error storing boolean expression index: %d"), new_expression->matches_size);
                  goto free_memory;
               }
               atom_index_string_size = strlen(atom_index_string);
            }
            /* atom_index_string_size is 0 if there was no atom expression,
             * otherwise holds the size. There is a +2 for next_char and EOL */
            if (strlen(expr) + atom_index_string_size + 2 > expr_string_size)
            {
               /* fprintf(stderr, "Enlarging expr '%s' from %d\n", expr, expr_string_size); */
               expr_string_size = expr_string_size*2;
               expr = (char *) realloc(expr, expr_string_size * sizeof(char));
               if (expr == NULL) { goto free_memory; }
            }
            if (atom != NULL)
            {
               strcat (expr, atom_index_string);
               atom = NULL;
            }
            if (next_char == '\0') { /* end of expression */ break; }
            next_char_string[0] = next_char;
            next_char_string[1] = '\0';
            strcat(expr, next_char_string);
         }
         else if (atom == NULL && ! isspace(next_char))
         { /* begin an atom expression */
            atom = new_expression->string+index;
         }
         index++;
      }
   }
   else
   {
      return NULL;
   }
   new_expression->expression_string = expr;
   /* fprintf(stderr, "%s\n", expr); */

   /* parse the boolean 'atoms' into name = value and store name and value
    * in the matches structures array */
   if ((new_expression->matches = malloc (new_expression->matches_size *
       sizeof(halevt_match))) == NULL)
   {
      goto free_memory;
   }
   for (i=0; i < new_expression->matches_size; i++)
   {
      char *match_property_string;
      halevt_match *new_match;

      new_match = &(new_expression->matches[i]);

      /* first parse a 'name = value' or a simple 'name', remove leading
         spaces from value and trailing spaces from name */
      new_match->value = atoms[i];

      match_property_string = strsep(&(new_match->value), "=");
      if (new_match->value != NULL)
      {
         while (isspace(*new_match->value)) { (new_match->value)++; }
         s = strlen (match_property_string);
         while (s > 0 && isspace (*(match_property_string + s-1))) { s--; };
         match_property_string[s] = '\0';
      }

      if ((halevt_hal_string(match_property_string)) == NULL)
      {
         DEBUG(_("Bad match expression: %s"), match_property_string);
         goto free_match;
      }

      halevt_parse_property_name(match_property_string, &(new_match->property));

      if (new_match->property.name == NULL)
         goto free_match;
   }

   /* now use boolstuff to parse the expression and transform to DNF */
   new_expression->tree = boolstuff_parse (expr, NULL, &bool_error);
   if (new_expression->tree == NULL)
   {
      if (bool_error == BOOLSTUFF_ERR_GARBAGE_AT_END)
      {
          DEBUG(_("Garbage at end of boolean expression \"%s\""), new_expression->string);
      }
      else if (bool_error == BOOLSTUFF_ERR_RUNAWAY_PARENTHESIS)
      {
          DEBUG(_("Runaway parenthesis in boolean expression \"%s\""), new_expression->string);
      }
      else if (bool_error == BOOLSTUFF_ERR_IDENTIFIER_EXPECTED)
      {
          DEBUG(_("String instead of operator expected in boolean expression \"%s\""), new_expression->string);
      }
      goto free_match;
   }
   free (atoms);
   new_expression->tree = boolstuff_get_disjunctive_normal_form (new_expression->tree);

   return new_expression;

free_match:
   free (new_expression->matches);
free_memory:
   free (expr);
   free (atoms);
   free (new_expression->string);
   free (new_expression);
   return NULL;
}

void halevt_free_boolean_expression (halevt_boolean_expression *expr)
{
   int i;

   free (expr->expression_string);
   free (expr->string);

   for (i = 0; i < expr->matches_size; i++)
   {
      free(expr->matches[i].property.parents);
   }

   boolstuff_destroy_tree (expr->tree);
   free (expr->matches);
   free (expr);
}

char *halevt_print_boolean_expression (halevt_boolean_expression *expr)
{
   char *string = NULL;
   char tmp[256];
   int total_size = 0;
   int i;
   char **match_properties_string;

   if ((match_properties_string = (char **) malloc(expr->matches_size * sizeof(char *))) == NULL)
      return NULL;

   for (i=0; i < expr->matches_size; i++)
   {
      match_properties_string[i] = halevt_print_property_name(&(expr->matches[i].property));
      if (match_properties_string[i] != NULL)
        total_size += strlen(match_properties_string[i]);
      if (expr->matches[i].value !=  NULL) 
        total_size += strlen(expr->matches[i].value) + 1;
      total_size += 2;
   }
   sprintf(tmp, "%d: ", expr->matches_size);
   /* +2 for the first ", ", +1 for the end of line  */
   total_size += strlen(tmp) + strlen(expr->expression_string) +2 +1;
   if ((string = (char *) malloc(total_size * sizeof(char))) == NULL)
      goto oom;
   strcpy (string, tmp);
   strcat (string, expr->expression_string);
   strcat(string, ", ");
   for (i=0; i < expr->matches_size; i++)
   {
      if (match_properties_string[i] != NULL)
        strcat(string, match_properties_string[i]);
      if (expr->matches[i].value != NULL)
      {
         strcat(string, "=");
         strcat(string, expr->matches[i].value);
      }
      strcat(string, ", ");
   }
oom:
   for (i=0; i < expr->matches_size; i++)
   {
      free (match_properties_string[i]);
   }
   free (match_properties_string);
   return string;
}

int halevt_value_true(const char *value, const char *udi,
   const halevt_device *device,
   const halevt_boolean_expression *expr)
{
    return halevt_matches(&(expr->matches[atoi(value)]), udi, device);
}

/* return 1 if the tree evaluate to true, 0 if the tree evaluates to false.
 * the tree should already be in DNF.
 *
 * If the udi is NULL, the device passed in argument will be used by
 * halevt_value_true
 * */
int halevt_true_tree(const halevt_boolean_expression *expr, const char* udi,
   const halevt_device *device)
{
    int or_root_number = 0;
    boolexpr_t current_boolean_expr;
    boolexpr_t *boolean_or_roots;
    char **positives;
    char **negatives;
    int result = 0;
    int i;

    boolean_or_roots = boolstuff_get_dnf_term_roots (expr->tree, NULL);
    current_boolean_expr = boolean_or_roots[0];
    while (current_boolean_expr != NULL)
    {
        boolstuff_get_tree_variables (current_boolean_expr, &positives, &negatives);
        for (i = 0; *(positives + i) != NULL; i++)
        {
            if (! halevt_value_true (*(positives + i), udi, device, expr)) { goto next_or; }
        }
        for (i = 0; *(negatives + i) != NULL; i++)
        {
            if (halevt_value_true (*(negatives + i), udi, device, expr)) { goto next_or; }
        }

        boolstuff_free_variables_sets (positives, negatives);
        result = 1;
        break;

next_or:
        boolstuff_free_variables_sets (positives, negatives);
        or_root_number++;
        current_boolean_expr = boolean_or_roots[or_root_number];
    }

    boolstuff_free_node_array (boolean_or_roots);
    return result;
}
