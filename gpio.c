/*
 * koruza-driver - KORUZA driver
 *
 * Copyright (C) 2017 Jernej Kos <jernej@kos.mx>
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
#include "gpio.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define GPIO_SYSFS_EXPORT "/sys/class/gpio/export"
#define GPIO_SYSFS_UNEXPORT "/sys/class/gpio/unexport"
#define GPIO_SYSFS_DIRECTION "/sys/class/gpio/gpio%d/direction"
#define GPIO_SYSFS_VALUE "/sys/class/gpio/gpio%d/value"

int gpio_configure(const char *interface, int pin);
int gpio_raw_write_pin(const char *interface, int pin, const char *value, size_t length);

int gpio_configure(const char *interface, int pin)
{
  int fd = open(interface, O_WRONLY);
  if (fd < 0) {
    return -1;
  }

  char buffer[3];
  ssize_t length;
  length = snprintf(buffer, sizeof(buffer), "%d", pin);
  if (write(fd, buffer, length) != length) {
    close(fd);
    return -1;
  }

  close(fd);
  return 0;
}

int gpio_raw_write_pin(const char *interface, int pin, const char *value, size_t length)
{
  char path[128];
  snprintf(path, sizeof(path), interface, pin);

  int fd = open(path, O_WRONLY);
  if (fd < 0) {
    return -1;
  }

  if (write(fd, value, length) != length) {
    close(fd);
    return -1;
  }

  close(fd);
  return 0;
}

int gpio_export(int pin)
{
  return gpio_configure(GPIO_SYSFS_EXPORT, pin);
}

int gpio_unexport(int pin)
{
  return gpio_configure(GPIO_SYSFS_UNEXPORT, pin);
}

int gpio_direction(int pin, int direction)
{
  return gpio_raw_write_pin(
    GPIO_SYSFS_DIRECTION,
    pin,
    (direction == GPIO_IN) ? "in" : "out",
    (direction == GPIO_IN) ? 2 : 3
  );
}

int gpio_read(int pin)
{
  char path[128];
  snprintf(path, sizeof(path), GPIO_SYSFS_VALUE, pin);

  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    return -1;
  }

  char buffer[3];
  if (read(fd, buffer, sizeof(buffer)) < 0) {
    close(fd);
    return -1;
  }

  close(fd);
  return atoi(buffer);
}

int gpio_write(int pin, int value)
{
  return gpio_raw_write_pin(GPIO_SYSFS_VALUE, pin, value == GPIO_LOW ? "0" : "1", 1);
}
