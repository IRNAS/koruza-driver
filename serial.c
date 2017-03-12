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
#include "serial.h"
#include "configuration.h"

#include <libubox/uloop.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <termios.h>
#include <string.h>
#include <errno.h>

// Serial device uloop file descriptor wrapper.
static struct uloop_fd serial_ufd;
// Frame parser.
static parser_t parser;

int serial_init_device(const char *device);
void serial_fd_handler(struct uloop_fd *ufd, unsigned int events);

int serial_init(struct uci_context *uci)
{
  int result = 0;
  serial_ufd.fd = -1;
  frame_parser_init(&parser);

  // Read device from configuration.
  char *device = uci_get_string(uci, "koruza.@mcu[0].device");
  if (device != NULL) {
    result = serial_init_device(device);
    free(device);
  } else {
    // Fallback to default device name.
    result = serial_init_device("/dev/ttyS1");
  }

  return result;
}

void serial_set_message_handler(frame_message_handler handler)
{
  parser.handler = handler;
}

int serial_init_device(const char *device)
{
  serial_ufd.fd = open(device, O_RDWR);
  if (serial_ufd.fd < 0) {
    syslog(LOG_ERR, "Failed to open serial device '%s'.", device);
    return -1;
  }

  struct termios serial_tio;
  if (tcgetattr(serial_ufd.fd, &serial_tio) < 0) {
    syslog(LOG_ERR, "Failed to get configuration for serial device '%s': %s (%d)",
      device, strerror(errno), errno);
    close(serial_ufd.fd);
    return -1;
  }

  cfmakeraw(&serial_tio);
  cfsetispeed(&serial_tio, B115200);
  cfsetospeed(&serial_tio, B115200);

  if (tcsetattr(serial_ufd.fd, TCSAFLUSH, &serial_tio) < 0) {
    syslog(LOG_ERR, "Failed to configure serial device '%s': %s (%d)",
      device, strerror(errno), errno);
    close(serial_ufd.fd);
    return -1;
  }

  serial_ufd.cb = serial_fd_handler;

  uloop_fd_add(&serial_ufd, ULOOP_READ);

  return 0;
}

void serial_fd_handler(struct uloop_fd *ufd, unsigned int events)
{
  (void) ufd;

  uint8_t buffer[1024];
  ssize_t size = read(serial_ufd.fd, buffer, sizeof(buffer));
  if (size < 0) {
    syslog(LOG_ERR, "Failed to read from serial device.");
    return;
  }

  frame_parser_push_buffer(&parser, buffer, size);
}

int serial_send_message(const message_t *message)
{
  if (serial_ufd.fd < 0) {
    return -1;
  }

  static uint8_t buffer[FRAME_MAX_LENGTH];
  ssize_t size = frame_message(buffer, sizeof(buffer), message);
  if (size < 0) {
    return -1;
  }

  size_t offset = 0;
  while (offset < size) {
    ssize_t written = write(serial_ufd.fd, &buffer[offset], size - offset);
    if (written < 0) {
      syslog(LOG_ERR, "Failed to write frame (%ld bytes) to serial device: %s (%d)",
        (long int) size, strerror(errno), errno);
      return -1;
    }

    offset += written;
  }

  return 0;
}
