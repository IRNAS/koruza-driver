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

#include <libubox/uloop.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <arpa/inet.h>

// Serial device uloop file descriptor wrapper.
static struct uloop_fd serial_ufd;
// Message handler for incoming messages.
static serial_message_handler message_handler = NULL;

int serial_init_device(const char *device);
void serial_fd_handler(struct uloop_fd *ufd, unsigned int events);

int serial_init(struct uci_context *uci)
{
  serial_ufd.fd = -1;

  // TODO: Read device from UCI configuration.
  return serial_init_device("/dev/ttyS1");
}

void serial_set_message_handler(serial_message_handler handler)
{
  message_handler = handler;
}

int serial_init_device(const char *device)
{
  serial_ufd.fd = open(device, O_RDWR);
  if (serial_ufd.fd < 0) {
    syslog(LOG_ERR, "Failed to open serial device '%s'.", device);
    return -1;
  }

  serial_ufd.cb = serial_fd_handler;

  uloop_fd_add(&serial_ufd, ULOOP_READ);

  return 0;
}

void serial_fd_handler(struct uloop_fd *ufd, unsigned int events)
{
  // TODO
}

int serial_send_message(message_t *message)
{
  if (serial_ufd.fd < 0) {
    return -1;
  }

  uint8_t buffer[65536];
  ssize_t size = message_serialize(buffer, sizeof(buffer), message);
  if (size < 0) {
    return -1;
  }

  uint16_t frame_size = htons((uint16_t) size);
  if (write(serial_ufd.fd, &frame_size, sizeof(frame_size)) < 0) {
    syslog(LOG_ERR, "Failed to write frame size to serial device.");
    return -1;
  }

  if (write(serial_ufd.fd, buffer, frame_size) < 0) {
    syslog(LOG_ERR, "Failed to write frame payload (%d bytes) to serial device.", size);
    return -1;
  }

  return 0;
}
