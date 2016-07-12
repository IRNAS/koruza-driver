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
#include "frame.h"

#include <stdio.h>
#include <stdlib.h>

static size_t number_parsed_messages = 0;

static void validate_message_handler(const message_t *message)
{
  printf("Received message from frame parser.\n");

  uint8_t parsed_command;
  tlv_motor_position_t parsed_position;
  if (message_tlv_get_command(message, &parsed_command) != MESSAGE_SUCCESS) {
    printf("Failed to get command TLV.\n");
    exit(1);
  }

  if (message_tlv_get_motor_position(message, &parsed_position) != MESSAGE_SUCCESS) {
    printf("Failed to get motor position TLV.\n");
    exit(1);
  }

  printf("Parsed command %u and motor position (%d, %d, %d)\n",
    parsed_command,
    parsed_position.x, parsed_position.y, parsed_position.z
  );

  if (parsed_command != COMMAND_MOVE_MOTOR ||
      parsed_position.x != 10 ||
      parsed_position.y != 20 ||
      parsed_position.z != -15) {
    printf("Parsed values are invalid.\n");
    exit(1);
  }

  number_parsed_messages++;
}

int main()
{
  message_t msg;
  message_init(&msg);
  message_tlv_add_command(&msg, COMMAND_MOVE_MOTOR);
  tlv_motor_position_t position = {10, 20, -15};
  message_tlv_add_motor_position(&msg, &position);
  message_tlv_add_checksum(&msg);

  // Frame message.
  uint8_t frame[1024];
  ssize_t frame_size = frame_message(frame, sizeof(frame), &msg);
  if (frame_size < 0) {
    printf("Failed to frame message!\n");
    return -1;
  }

  // Parse framed message.
  parser_t parser;
  frame_parser_init(&parser);
  parser.handler = validate_message_handler;
  frame_parser_push_byte(&parser, 0x10);
  frame_parser_push_byte(&parser, 0x20);
  frame_parser_push_byte(&parser, FRAME_MARKER_ESCAPE);
  frame_parser_push_byte(&parser, FRAME_MARKER_START);
  frame_parser_push_byte(&parser, 0x15);
  frame_parser_push_byte(&parser, FRAME_MARKER_END);
  frame_parser_push_buffer(&parser, frame, frame_size);
  frame_parser_push_byte(&parser, 0x10);
  frame_parser_free(&parser);

  if (number_parsed_messages != 1) {
    printf("Failed to parse exactly one message.\n");
    return -1;
  }

  message_free(&msg);

  return 0;
}
