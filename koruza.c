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
#include <syslog.h>
#include <libubox/uloop.h>

// Status of the connected KORUZA unit.
static struct koruza_status status;
// Timer for periodic status retrieval.
struct uloop_timeout timer_status;

void koruza_serial_message_handler(const message_t *message);
void koruza_timer_status_handler(struct uloop_timeout *timer);

int koruza_init(struct uci_context *uci)
{
  memset(&status, 0, sizeof(struct koruza_status));
  serial_set_message_handler(koruza_serial_message_handler);

  // Setup status timer handler.
  timer_status.cb = koruza_timer_status_handler;
  // TODO: Make the period configurable via UCI.
  uloop_timeout_set(&timer_status, 10000);

  return koruza_update_status();
}

const struct koruza_status *koruza_get_status()
{
  return &status;
}

void koruza_serial_message_handler(const message_t *message)
{
  // TODO: Remove this debug print.
  message_print(message);

  // Check if this is a reply message.
  tlv_reply_t reply;
  if (message_tlv_get_reply(message, &reply) != MESSAGE_SUCCESS) {
    return;
  }

  switch (reply) {
    case REPLY_STATUS_REPORT: {
      if (!status.connected) {
        // Was not considered connected until now.
        syslog(LOG_INFO, "Detected KORUZA MCU on the configured serial port.");
        status.connected = 1;
      }

      // Handle motor position report.
      tlv_motor_position_t position;
      if (message_tlv_get_motor_position(message, &position) == MESSAGE_SUCCESS) {
        status.motors.x = position.x;
        status.motors.y = position.y;
        status.motors.z = position.z;
      }

      // TODO: Other reports.
      break;
    }
  }
}

int koruza_move_motor(int32_t x, int32_t y, int32_t z)
{
  if (!status.connected) {
    return -1;
  }

  tlv_motor_position_t position;
  position.x = x;
  position.y = y;
  position.z = z;

  message_t msg;
  message_init(&msg);
  message_tlv_add_command(&msg, COMMAND_MOVE_MOTOR);
  message_tlv_add_motor_position(&msg, &position);
  message_tlv_add_checksum(&msg);
  serial_send_message(&msg);
  message_free(&msg);

  return 0;
}

int koruza_update_status()
{
  message_t msg;
  message_init(&msg);
  message_tlv_add_command(&msg, COMMAND_GET_STATUS);
  message_tlv_add_checksum(&msg);
  serial_send_message(&msg);
  message_free(&msg);

  return 0;
}

void koruza_timer_status_handler(struct uloop_timeout *timeout)
{
  (void) timeout;

  koruza_update_status();

  // TODO: Make the period configurable via UCI.
  uloop_timeout_set(&timer_status, 10000);
}
