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
#ifndef KORUZA_DRIVER_UCI_H
#define KORUZA_DRIVER_UCI_H

#include <uci.h>

/**
 * Returns a resolved UCI path as string. The caller is required to
 * free the string after use.
 *
 * @param uci UCI context
 * @param location UCI location expression (extended syntax)
 * @return Target string or NULL
 */
char *uci_get_string(struct uci_context *uci, const char *location);

/**
 * Returns a resolved UCI path as an integer.
 *
 * @param uci UCI context
 * @param location UCI location expression (extended syntax)
 * @param def Default value
 * @return Target integer
 */
int uci_get_int(struct uci_context *uci, const char *location, int def);

/**
 * Returns a resolved UCI path as a float.
 *
 * @param uci UCI context
 * @param location UCI location expression (extended syntax)
 * @param def Default value
 * @return Target float
 */
float uci_get_float(struct uci_context *uci, const char *location, float def);

/**
 * Stores a new string value into UCI configuration.
 *
 * @param uci UCI context
 * @param location UCI location expression (extended syntax)
 * @param value Value to store
 */
void uci_set_string(struct uci_context *uci, const char *location, const char *value);

/**
 * Stores a new integer value into UCI configuration.
 *
 * @param uci UCI context
 * @param location UCI location expression (extended syntax)
 * @param value Value to store
 */
void uci_set_int(struct uci_context *uci, const char *location, int value);

/**
 * Deletes a specific UCI pointer.
 *
 * @param uci UCI context
 * @param location UCI location expression (extended syntax)
 */
void uci_delete_ptr(struct uci_context *uci, const char *location);

#endif
