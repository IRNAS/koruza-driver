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
#include <libubus.h>

struct koruza_motor_status {
  int32_t x;
  int32_t y;
  int32_t z;
  uint32_t range_x;
  uint32_t range_y;
};

struct koruza_camera_calibration {
  uint32_t width;
  uint32_t height;
  uint32_t offset_x;
  uint32_t offset_y;
  uint32_t distance;
};

struct koruza_error_report {
  uint32_t code;
};

struct koruza_status {
  uint8_t connected;

  struct koruza_error_report errors;
  struct koruza_motor_status motors;
  struct koruza_camera_calibration camera_calibration;
};

int koruza_init(struct uci_context *uci, struct ubus_context *ubus);
int koruza_move_motor(int32_t x, int32_t y, int32_t z);
int koruza_homing();
int koruza_update_status();
const struct koruza_status *koruza_get_status();

#endif
