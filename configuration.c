/*
 * koruza-driver - KORUZA driver
 *
 * Copyright (C) 2016 Jernej Kos <jernej@kos.mx>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "configuration.h"

#include <stdlib.h>
#include <string.h>

char *uci_get_string(struct uci_context *uci, const char *location)
{
  struct uci_ptr ptr;
  char *loc = strdup(location);
  char *result = NULL;

  // Perform an UCI extended lookup.
  if (uci_lookup_ptr(uci, &ptr, loc, true) != UCI_OK) {
    free(loc);
    return NULL;
  }

  if (ptr.o && ptr.o->type == UCI_TYPE_STRING) {
    result = strdup(ptr.o->v.string);
  }

  free(loc);
  return result;
}

int uci_get_int(struct uci_context *uci, const char *location, int def)
{
  char *string = uci_get_string(uci, location);
  if (!string)
    return def;

  int result = atoi(string);
  free(string);

  return result;
}

void uci_set_string(struct uci_context *uci, const char *location, const char *value)
{
  struct uci_ptr ptr;
  char *loc = strdup(location);

  // Perform an UCI extended lookup.
  if (uci_lookup_ptr(uci, &ptr, loc, true) != UCI_OK) {
    free(loc);
    return;
  }

  ptr.value = value;
  uci_set(uci, &ptr);
  free(loc);
}

void uci_set_int(struct uci_context *uci, const char *location, int value)
{
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%d", value);
  uci_set_string(uci, location, buffer);
}
