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
#ifndef KORUZA_DRIVER_GPIO_H
#define KORUZA_DRIVER_GPIO_H

// Directions.
#define GPIO_IN 0
#define GPIO_OUT 1

// States.
#define GPIO_LOW 0
#define GPIO_HIGH 1

/**
 * Export GPIO pin via sysfs interface.
 *
 * @param pin Pin to export
 * @return Zero on success, -1 on failure
 */
int gpio_export(int pin);

/**
 * Unexport GPIO pin via sysfs interface.
 *
 * @param pin Pin to unexport
 * @return Zero on success, -1 on failure
 */
int gpio_unexport(int pin);

/**
 * Set GPIO pin direction.
 *
 * @param pin Pin to direction to
 * @param direction Direction (either GPIO_IN or GPIO_OUT)
 * @return Zero on success, -1 on failure
 */
int gpio_direction(int pin, int direction);

/**
 * Reads state from the given GPIO pin.
 *
 * @param pin Pin to read from
 * @return Value
 */
int gpio_read(int pin);

/**
 * Sets state of the given GPIO pin.
 *
 * @param pin Pin to write to
 * @param value State to set (either GPIO_LOW or GPIO_HIGH)
 * @return Zero on success, -1 on failure
 */
int gpio_write(int pin, int value);

#endif
