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
#include "koruza.h"
#include "serial.h"

#include <string.h>

// Status of the connected KORUZA unit.
static struct koruza_status status;

void koruza_serial_message_handler(message_t *message);

int koruza_init(struct uci_context *uci)
{
  memset(&status, 0, sizeof(struct koruza_status));
  serial_set_message_handler(koruza_serial_message_handler);

  return koruza_update_status();
}

void koruza_serial_message_handler(message_t *message)
{
  // TODO: Handle incoming message.
}

int koruza_move_motor(int32_t x, int32_t y, int32_t z)
{
  if (!status.connected) {
    return -1;
  }

  return 0;
}

int koruza_update_status()
{
  return 0;
}