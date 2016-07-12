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
#ifndef KORUZA_DRIVER_KORUZA_H
#define KORUZA_DRIVER_KORUZA_H

#include <uci.h>

struct koruza_motor_status {
  int32_t x;
  int32_t y;
  int32_t z;
};

struct koruza_status {
  uint8_t connected;

  struct koruza_motor_status motors;
};

int koruza_init(struct uci_context *uci);
int koruza_move_motor(int32_t x, int32_t y, int32_t z);
int koruza_update_status();

#endif
