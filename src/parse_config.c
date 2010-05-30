/*
    parse_config.c parse the config file
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

#include <libxml/tree.h>
#include <stdio.h>
#include <string.h>
#include <libintl.h>

#include "common.h"
#include "parse_config.h"
#include "match.h"

halevt_property *halevt_property_root = NULL;
halevt_insertion *halevt_insertion_root = NULL;
halevt_removal *halevt_removal_root = NULL;
halevt_condition *halevt_condition_root = NULL;
halevt_oninit *halevt_oninit_root = NULL;

static const char *halevt_hal_prefix = "hal.";
static const char *halevt_hal_exec_prefix = "$hal.";
static const char *hal_substring = ".hal.";

/*
 * returns the beginning of the string after the 'hal prefix', if
 * the string begins with this 'hal prefix', NULL otherwise.
 */
char *halevt_hal_string (char *string)
{
   if (strlen(string) < 5 || strncmp(string, halevt_hal_prefix, strlen(halevt_hal_prefix)))
   {
       return NULL;
   }
   return string + strlen(halevt_hal_prefix);
}

/* cut the string in substring separated by hal. the last
   corresponds with the value, the other are the parents */

void halevt_parse_property_name (char *string, halevt_property_name *property_name)
{
   char *p;
   int parents_size = 0;   

   property_name->name = string;

   /* first find the number of parents */
   p = string;
   while ((p = strstr(p, hal_substring)) != NULL)
   {
      parents_size++;
      p = p + 1;
   }
   if ((property_name->parents = malloc(sizeof(char *) * (parents_size + 1))) == NULL)
   {
     property_name->name = NULL;
     return;
   }
   if (parents_size > 0)
   {
      int parent_index = 0;
      int j;
      char *string;
      p = property_name->name;
      property_name->parents[0] = property_name->name;
      /* cut the string in pieces */
      while ((p = strstr (p, hal_substring)) != NULL)
      {
         *p = '\0';
         parent_index++;
         property_name->parents[parent_index] = p+1;
         p = p + 1;
      }

      for (j=0; j<parent_index+1; j++)
      {
        if ((string = halevt_hal_string(property_name->parents[j])) == NULL)
        {
            DEBUG(_("Bad match expression: %s"), property_name->parents[j]);
            property_name->name = NULL;
            free(property_name->parents);
            return;
        }
        property_name->parents[j] = string;
     }
     property_name->name = property_name->parents[parent_index];
   }
   else
   {
      property_name->name = halevt_hal_string(property_name->name);
   }
   property_name->parents[parents_size] = NULL;
}

halevt_exec *halevt_new_exec(const xmlChar *exec)
{
   char *string;
   char *parsed_string;
   char *p;
   halevt_exec *result_exec;
   int sub_elements_number = 0;
   if ((string = strdup((char *)exec)) == NULL)
      return NULL;

   /* handle the special case where there is a $hal. at the beginning */
   if (strlen(string) >= strlen(halevt_hal_exec_prefix) && !strncmp(halevt_hal_exec_prefix, string, strlen(halevt_hal_exec_prefix)))
   {
      DEBUG(_("Bad exec string: %s"), string);
      free(string);
      return NULL;
   }
   if ((parsed_string = strdup(string)) == NULL)
   {
      free (string);
      return NULL;
   }
   if ((result_exec = (halevt_exec *) malloc(sizeof(halevt_exec))) == NULL)
   {
      free (string);
      free (parsed_string);
      return NULL;
   }

   /* first pass to determine the size of the array of substring */
   p = parsed_string;
   while ((p = strstr(p, halevt_hal_exec_prefix)) != NULL)
   {
      p = p+1;
      if ((p = strstr(p, "$")) != NULL)
      {
         sub_elements_number++;
      }
      else
      {
         break;
      }
      if (*(p+1) == '\0')
      {
         break;
      }
      sub_elements_number++;
   }
   /* now allocate the array */
   if ((result_exec->elements = (halevt_exec_element *) malloc(sizeof(halevt_exec_element) * (sub_elements_number +1)))  == NULL)
   {
      free (string);
      free (parsed_string);
      free (result_exec);
      return NULL;
   }
   /* fill the array */
   sub_elements_number = 0;
   p = parsed_string;
   result_exec->elements[0].string = parsed_string;
   result_exec->elements[0].hal_property = 0;

   while ((p = strstr(p, halevt_hal_exec_prefix)) != NULL)
   {
      char *p_begin = p;
      p = p+1;
      if ((p = strstr(p, "$")) != NULL)
      {
         sub_elements_number++;
         *p = '\0';
         *p_begin = '\0';
         halevt_parse_property_name(p_begin+strlen("$"), &(result_exec->elements[sub_elements_number].property));
         if (result_exec->elements[sub_elements_number].property.name == NULL)
         {
           free (string);
           free (parsed_string);
           free (result_exec);
           return NULL;
         }
         /* result_exec->elements[sub_elements_number].string = p_begin+strlen(halevt_hal_exec_prefix); */
         result_exec->elements[sub_elements_number].hal_property = 1;
         result_exec->elements[sub_elements_number].string = NULL;
      }
      else
      {
         break;
      }
      if (*(p+1) == '\0')
      {
         break;
      }
      p = p+1;
      sub_elements_number++;
      result_exec->elements[sub_elements_number].string = p;
      result_exec->elements[sub_elements_number].property.name = NULL;
      result_exec->elements[sub_elements_number].hal_property = 0;
   }
   /*result_exec->elements[sub_elements_number+1] = NULL;*/
   result_exec->exec_size = sub_elements_number+1;
   result_exec->string = string;
   result_exec->parsed_string = parsed_string;
   return result_exec;
}

static void halevt_free_exec(halevt_exec *exec)
{
   int i;
   free(exec->string);
   free(exec->parsed_string);
   for (i = 0; i < exec->exec_size; i++)
   {
     if (exec->elements[i].hal_property == 1)
     {
       free (exec->elements[i].property.parents);
     }
   }
   free(exec->elements);
   free(exec);
}

/* for debugging */
char *halevt_print_property_name(halevt_property_name *property)
{
   int total_size = 0;
   char *string;
   char **parent;
   if (property->name == NULL)
     return NULL;
   total_size += strlen(property->name)+1;
   WALK_NULL_ARRAY(parent, property->parents)
   {
     total_size += strlen(*parent) +2;
   }
   if ((string = (char *) malloc(total_size * sizeof(char))) == NULL)
      return NULL;
   *string = '\0';
   WALK_NULL_ARRAY(parent, property->parents)
   {
     strcat(string, *parent);
     strcat(string, "->");
   }
   strcat(string, property->name);
   return string;
}

/* for debugging */
char *halevt_print_exec(halevt_exec *exec_str)
{
   int i;
   int str_size = 0;
   char *result;
   /* used to store the formatted property strings */
   char **property_strings;

   if ((property_strings = (char **) malloc(exec_str->exec_size * sizeof (char*))) == NULL)
     return NULL;
   
   for(i = 0; i < exec_str->exec_size; i++)
   {
      if (exec_str->elements[i].hal_property)
      {
         property_strings[i] = halevt_print_property_name (&(exec_str->elements[i].property));
         if (property_strings[i] != NULL)
           str_size += strlen(property_strings[i]);
      }
      else
      {
        str_size += strlen(exec_str->elements[i].string);
        /* not useful, but cleaner */
        property_strings[i] = NULL;
      }
      str_size += 2;
   }
   if ((result = (char *) malloc((str_size+1)*sizeof(char))) == NULL)
   {
       return NULL;
   }
   *result = '\0';
   for(i = 0; i < exec_str->exec_size; i++)
   {
      if (exec_str->elements[i].hal_property && property_strings[i] != NULL)
      {
         strcat (result, property_strings[i]);
         free (property_strings[i]);
      }
      else
      {
         strcat (result, exec_str->elements[i].string);
      }
      strcat(result, "| ");
   }
   free (property_strings);
   return result;
}

halevt_insertion *halevt_add_insertion(const xmlChar *match, const xmlChar *exec)
{
   halevt_insertion *new_insertion;
   new_insertion = malloc(sizeof(halevt_insertion));
   if (new_insertion != NULL)
   {
      if ((new_insertion->match = halevt_new_boolean_expression(match)) == NULL)
      {
         free(new_insertion);
         return NULL;
      }
      if ((new_insertion->exec = halevt_new_exec(exec)) == NULL)
      {
         halevt_free_boolean_expression (new_insertion->match);
         free(new_insertion);
         return NULL;
      }

/*
      printf ("add_insertion %s, %s\n", match, exec);
*/

      new_insertion->next = halevt_insertion_root;
      halevt_insertion_root = new_insertion;
   }
   return new_insertion;
}

static void halevt_free_insertion(halevt_insertion *insertion)
{
   halevt_free_boolean_expression(insertion->match);
   halevt_free_exec(insertion->exec);
   free(insertion);
}

halevt_oninit *halevt_add_oninit(const xmlChar *match, const xmlChar *exec)
{
   halevt_oninit *new_oninit;
   new_oninit = malloc(sizeof(halevt_oninit));
   if (new_oninit != NULL)
   {
      if ((new_oninit->match = halevt_new_boolean_expression (match)) == NULL)
      {
         free(new_oninit);
         return NULL;
      }
      if ((new_oninit->exec = halevt_new_exec (exec)) == NULL)
      {
         halevt_free_boolean_expression (new_oninit->match);
         free(new_oninit);
         return NULL;
      }
/*
      printf ("add_oninit %s, %s\n", match, exec);
*/
      new_oninit->next = halevt_oninit_root;
      halevt_oninit_root = new_oninit;
   }
   return new_oninit;
}

static void halevt_free_oninit(halevt_oninit *oninit)
{
   halevt_free_boolean_expression(oninit->match);
   halevt_free_exec(oninit->exec);
   free(oninit);
}

halevt_removal *halevt_add_removal(const xmlChar *match, const xmlChar *exec)
{
   halevt_removal *new_removal;
   new_removal = malloc(sizeof(halevt_removal));
   if (new_removal != NULL)
   {
      if ((new_removal->match = halevt_new_boolean_expression(match)) == NULL)
      {
         free (new_removal);
         return NULL;
      }
      if ((new_removal->exec = halevt_new_exec(exec)) == NULL)
      {
         halevt_free_boolean_expression (new_removal->match);
         free (new_removal);
         return NULL;
      }
/*
      printf ("add_removal %s, %s\n", match, exec);
*/
      new_removal->next = halevt_removal_root;
      halevt_removal_root = new_removal;
   }
   return new_removal;
}

static void halevt_free_removal(halevt_removal *removal)
{
   halevt_free_boolean_expression(removal->match);
   halevt_free_exec(removal->exec);
   free(removal);
}

halevt_condition *halevt_add_condition(const xmlChar *match,
    const xmlChar *exec, const xmlChar *name, const xmlChar *value)
{
   halevt_condition *new_condition;
   new_condition = malloc (sizeof(halevt_condition));
   if (new_condition != NULL)
   {
      if ((new_condition->match = halevt_new_boolean_expression(match)) == NULL)
      {
         free(new_condition);
         return NULL;
      }
      if ((new_condition->name = (char *) xmlStrdup(name)) == NULL)
      {
         halevt_free_boolean_expression (new_condition->match);
         free(new_condition);
         return NULL;
      }
      if (value == NULL) { new_condition->value = NULL; }
      else if ((new_condition->value = (char *) xmlStrdup(value)) == NULL)
      {
         halevt_free_boolean_expression (new_condition->match);
         free(new_condition->name);
         free(new_condition);
         return NULL;
      }
      if ((new_condition->exec = halevt_new_exec(exec)) == NULL)
      {
         halevt_free_boolean_expression (new_condition->match);
         free(new_condition->name);
         if (new_condition->value != NULL)
            free(new_condition->value);
         free(new_condition);
         return NULL;
      }
/*
      printf ("add_condition %s, %s, %s, %s\n", match, exec, name, value);
*/
      new_condition->next = halevt_condition_root;
      halevt_condition_root = new_condition;
   }
   return new_condition;
}

static void halevt_free_condition(halevt_condition *condition)
{
   halevt_free_boolean_expression(condition->match);
   halevt_free_exec(condition->exec);
   free(condition->name);
   free(condition);
}

/* match: the device match 
   name: the property name
*/
halevt_property *halevt_add_property(const xmlChar *match, xmlChar *name)
{
   halevt_property *new_property;
   new_property = malloc(sizeof(halevt_property));
   if (new_property != NULL)
   {
      char *hal_property_name = halevt_hal_string((char *) name);
      if (hal_property_name == NULL)
      {
         DEBUG(_("Invalid Property name: %s"), name);
         free(new_property);
         return NULL;
      }
      if ((new_property->name = strdup(hal_property_name)) == NULL)
      {
         free(new_property);
         return NULL;
      }
      if ((new_property->match = halevt_new_boolean_expression(match)) == NULL)
      {
         free(new_property->name);
         free(new_property);
         return NULL;
      }

/*
      printf ("add_property %s, %s\n", name, match);
*/
      new_property->next = halevt_property_root;
      new_property->action = NULL;
      halevt_property_root = new_property;
   }
   return new_property;
}

halevt_property_action *halevt_add_property_value(halevt_property *property,
    const xmlChar *exec, const xmlChar *value)
{
   halevt_property_action *new_property_action;
   new_property_action = malloc (sizeof(halevt_property_action));

   if (new_property_action != NULL)
   {
      if ((new_property_action->value = (char *) xmlStrdup(value)) == NULL)
      {
         free (new_property_action);
         return NULL;
      }
      if ((new_property_action->exec = halevt_new_exec(exec)) == NULL)
      {
         free (new_property_action->value);
         free (new_property_action);
         return NULL;
      }
/*
      printf ("add_property_value %s, %s, %s\n", property->name, exec, value);
*/
      new_property_action->next = property->action;
      property->action = new_property_action;
   }
   return new_property_action;
}

static void halevt_free_property_value(halevt_property_action *action)
{
   halevt_free_exec(action->exec);
   free(action->value);
   free(action);
}

static void halevt_free_property(halevt_property *property)
{
   FREE_LINKED_LIST(halevt_property_action, property->action, halevt_free_property_value);
   halevt_free_boolean_expression(property->match);
   free(property->name);
   free(property);
}

int halevt_parse_config (char const *path)
{
   xmlDocPtr doc;
   xmlNodePtr cur;
   xmlNodePtr device;
   char *root_node = "Configuration";

   doc = xmlParseFile (path);
   if (doc == NULL)
   {
       DEBUG(_("Document not parsed successfully"));
       return 0;
   }
   cur = xmlDocGetRootElement (doc);
   if (cur == NULL)
   {
       DEBUG(_("Document is empty"));
       xmlFreeDoc(doc);
       return 0;
   }
   if (xmlStrcmp (cur->name, (const xmlChar *) root_node))
   {
       DEBUG(_("Incorrect document type, root node should have been %s"),
             root_node);
       xmlFreeDoc (doc);
       return 0;
   }
   device = cur->children;
   while (device != NULL)
   {
      if (device->type == XML_TEXT_NODE || device->type == XML_COMMENT_NODE) { ; }
      else if (xmlStrcmp (device->name, (const xmlChar *) "Device"))
      {
          DEBUG(_("Warning: Incorrect element '%s', should be '%s'"), device->name, "Device");
      }
      else
      {
         xmlChar *match = xmlGetProp (device, (const xmlChar *) "match");
         if (match == NULL)
         {
            DEBUG(_("Warning: %s XML tag encountered with missing or bad attributes, ignored"), device->name);
         }
         else
         {
            cur = device->children;
            while (cur != NULL)
            {
               if (cur->type == XML_TEXT_NODE || cur->type == XML_COMMENT_NODE) { ; }
               else if (! xmlStrcmp (cur->name, (const xmlChar *) "Insertion"))
               {
                  xmlChar *exec = xmlGetProp (cur, (const xmlChar *) "exec");
                  if (exec == NULL)
                  {
                     DEBUG(_("Warning: %s XML tag encountered with missing or bad attributes, ignored"), cur->name);
                  }
                  else
                  {
                     halevt_insertion *new_insertion = halevt_add_insertion (match, exec);
                     if (new_insertion == NULL)
                       DEBUG(_("Warning: match %s, exec %s: adding insertion failed"), match, exec);
                     xmlFree(exec);
                  }
               }
               else if (! xmlStrcmp (cur->name, (const xmlChar *) "Removal"))
               {
                  xmlChar *exec = xmlGetProp (cur, (const xmlChar *) "exec");
                  if (exec == NULL)
                  {
                     DEBUG(_("Warning: %s XML tag encountered with missing or bad attributes, ignored"), cur->name);
                  }
                  else
                  {
                     halevt_removal *new_removal = halevt_add_removal (match, exec);
                     if (new_removal == NULL)
                       DEBUG(_("Warning: match %s, exec %s: adding removal failed"), match, exec);

                     xmlFree(exec);
                  }
               }
               else if (! xmlStrcmp (cur->name, (const xmlChar *) "OnInit"))
               {
                  xmlChar *exec = xmlGetProp (cur, (const xmlChar *) "exec");
                  if (exec == NULL)
                  {
                     DEBUG(_("Warning: %s XML tag encountered with missing or bad attributes, ignored"), cur->name);
                  }
                  else
                  {
                     halevt_oninit *new_oninit = halevt_add_oninit (match, exec);
                     if (new_oninit == NULL)
                       DEBUG(_("Warning: match %s, exec %s: adding action on init failed"), match, exec);
                     xmlFree(exec);
                  }
               }
               else if (! xmlStrcmp(cur->name, (const xmlChar *) "Condition"))
               {
                  xmlChar *exec =  xmlGetProp(cur, (const xmlChar *) "exec");
                  xmlChar *name =  xmlGetProp(cur, (const xmlChar *) "name");
                  xmlChar *value =  xmlGetProp(cur, (const xmlChar *) "value");
                  if (exec == NULL || name == NULL)
                  {
                     DEBUG(_("Warning: %s XML tag encountered with missing or bad attributes, ignored"), cur->name);
                  }
                  else
                  {
                     halevt_condition *new_condition = halevt_add_condition(match, exec, name, value);
                     if (new_condition == NULL)
                       DEBUG(_("Warning: match %s, exec %s (%s = %s): adding condition failed"), match, exec, name, value);
                  }
                  xmlFree(exec);
                  xmlFree(name);
                  xmlFree(value);
               }
               else if (! xmlStrcmp(cur->name, (const xmlChar *) "Property"))
               {
                  xmlChar *name = xmlGetProp(cur, (const xmlChar *) "name");
                  if (name == NULL)
                  {
                     DEBUG(_("Warning: %s XML tag encountered with missing or bad attributes, ignored"), cur->name);
                  }
                  else
                  {
                     halevt_property *new_property = halevt_add_property(match, name);

                     if (new_property  == NULL)
                     {
                        DEBUG(_("Warning: match %s, name %s: adding property failed"), match, name);
                     }
                     else
                     {
                        xmlNodePtr property_value_node = cur->children;
                        while(property_value_node != NULL)
                        {
                           if (property_value_node->type == XML_TEXT_NODE || property_value_node->type == XML_COMMENT_NODE) { ; }
                           else if (xmlStrcmp(property_value_node->name, (const xmlChar *) "Action"))
                           {
                              DEBUG(_("Warning: Incorrect Property element '%s', should be '%s'"), property_value_node->name, "Action");
                           }
                           else
                           {
                              xmlChar *exec = xmlGetProp(property_value_node, (const xmlChar *) "exec");
                              xmlChar *value = xmlGetProp(property_value_node, (const xmlChar *) "value");
                              if (exec == NULL || value == NULL)
                              {
                                 DEBUG(_("Warning: %s XML tag encountered with missing or bad attributes, ignored"), property_value_node->name);
                              }
                              else
                              {
                                 halevt_property_action *new_property_action = halevt_add_property_value(new_property, exec, value);
                                 if (new_property_action == NULL)
                                   DEBUG(_("Warning: match %s, name %s, value %s, exec %s: adding property value failed"), match, name, exec, value);
                              }
                              xmlFree(exec);
                              xmlFree(value);
                           }
                           property_value_node = property_value_node->next;
                        }
                     }
                  }
                  xmlFree(name);
               }
               else
               {
                  DEBUG(_("Warning: Incorrect element '%s'"), cur->name);
               }
               cur = cur->next;
            }
         }
         xmlFree(match);
      }
      device = device->next;
   }
   xmlFreeDoc(doc);
   return 1;
}

/* for debugging */
void halevt_print_config()
{
   halevt_insertion *insertion;
   halevt_removal *removal;
   halevt_condition *condition;
   halevt_property *property;

   insertion = halevt_insertion_root;
   while (insertion != NULL)
   {
      char *exec_string = halevt_print_exec(insertion->exec);
      char *boolean_expression = halevt_print_boolean_expression(insertion->match);
      fprintf(stderr, "insertion %s, %s\n", boolean_expression, exec_string);
      free(exec_string);
      free(boolean_expression);
      insertion = insertion->next;
   }
   removal = halevt_removal_root;
   while (removal != NULL)
   {
      char *boolean_expression = halevt_print_boolean_expression(removal->match);
      char *exec_string = halevt_print_exec(removal->exec);
      fprintf(stderr, "removal %s, %s\n", boolean_expression, exec_string);
      free (boolean_expression);
      free (exec_string);
      removal = removal->next;
   }
   condition = halevt_condition_root;
   while (condition != NULL)
   {
      char *boolean_expression = halevt_print_boolean_expression(condition->match);
      char *exec_string = halevt_print_exec(condition->exec);
      fprintf(stderr, "condition %s, %s, %s, %s\n", boolean_expression, exec_string, condition->name, condition->value);
      free (boolean_expression);
      free (exec_string);
      condition = condition->next;
   }
   property = halevt_property_root;
   while (property != NULL)
   {
      halevt_property_action *property_action = property->action;
      char *boolean_expression = halevt_print_boolean_expression(property->match);
      fprintf(stderr, "property %s, %s\n", property->name, boolean_expression);
      free (boolean_expression);
      while (property_action != NULL)
      {
         char *exec_string = halevt_print_exec(property_action->exec);
         fprintf(stderr, "  property_action %s, %s\n", property_action->value, exec_string);
         free (exec_string);
         property_action = property_action->next;
      }
      property = property->next;
   }
}

void halevt_free_config()
{
   FREE_LINKED_LIST(halevt_property, halevt_property_root, halevt_free_property);
   FREE_LINKED_LIST(halevt_insertion, halevt_insertion_root, halevt_free_insertion);
   FREE_LINKED_LIST(halevt_removal, halevt_removal_root, halevt_free_removal);
   FREE_LINKED_LIST(halevt_condition, halevt_condition_root, halevt_free_condition);
   FREE_LINKED_LIST(halevt_oninit, halevt_oninit_root, halevt_free_oninit);
   xmlCleanupParser();
}
