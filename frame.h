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
#ifndef KORUZA_DRIVER_FRAME_H
#define KORUZA_DRIVER_FRAME_H

#include "message.h"

/**
 * Handler for messages received in frames. After the message is handled, it will
 * be freed, so no external references should be kept.
 */
typedef void (*frame_message_handler)(const message_t *message);

/**
 * Parser states.
 */
typedef enum {
  SERIAL_STATE_WAIT_START = 0,
  SERIAL_STATE_WAIT_START_ESCAPE,
  SERIAL_STATE_IN_FRAME,
  SERIAL_STATE_AFTER_ESCAPE,
} parser_state_t;

#define FRAME_MAX_LENGTH 131070
#define FRAME_MARKER_START 0xF1
#define FRAME_MARKER_END 0xF2
#define FRAME_MARKER_ESCAPE 0xF3

/**
 * Frame parser.
 */
typedef struct {
  /// Handler that will be used to emit parsed messages.
  frame_message_handler handler;

  // Internal parser state.
  parser_state_t state;
  uint8_t *buffer;
  size_t buffer_size;
  size_t length;
} parser_t;

/**
 * Initializes the frame parser.
 *
 * @param parser Parser instance
 */
void frame_parser_init(parser_t *parser);

/**
 * Frees the frame parser.
 *
 * @param parser Parser instance
 */
void frame_parser_free(parser_t *parser);

/**
 * Pushes a buffer to the frame parser.
 *
 * @param parser Parser instance
 * @param buffer Buffer to push
 * @param length Size of the buffer
 */
void frame_parser_push_buffer(parser_t *parser, uint8_t *buffer, size_t length);

/**
 * Pushes a single byte to the frame parser.
 *
 * @param parser Parser instance
 * @param byte Byte to push
 */
void frame_parser_push_byte(parser_t *parser, uint8_t byte);

/**
 * Frames the given message.
 *
 * @param frame Destination buffer
 * @param length Destination buffer length
 * @param message Message to frame
 * @return Size of the output frame
 */
ssize_t frame_message(uint8_t *frame, size_t length, const message_t *message);

#endif
