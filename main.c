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
#include "message.h"

#include <stdio.h>

int main()
{
  message_t msg;
  message_init(&msg);
  message_tlv_add_command(&msg, COMMAND_GET_STATUS);
  message_tlv_add_checksum(&msg);

  printf("Generated protocol message: ");
  message_print(&msg);
  printf("\n");

  uint8_t buffer[1024];
  size_t length = message_serialize(buffer, 1024, &msg);
  printf("Serialized protocol message:\n");
  for (size_t i = 0; i < length; i++) {
    printf("%02X ", buffer[i]);
  }
  printf("\n");

  message_t msg_parsed;
  message_result_t result = message_parse(&msg_parsed, buffer, length);
  if (result == MESSAGE_SUCCESS) {
    printf("Parsed protocol message: ");
    message_print(&msg_parsed);
    printf("\n");
  } else {
    printf("Failed to parse serialized message: %d\n", result);
  }

  message_free(&msg);

  return 0;
}
