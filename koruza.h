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

// Survey resolution (number of bins in each direction).
#define SURVEY_BINS 100
// Survey coverage (motor coordinate distance from center to edge of survey).
#define SURVEY_COVERAGE 10000

// Accelerometer statistics window size (in number of samples).
#define ACCELEROMETER_STATISTICS_BUFFER_SIZE 120

struct accelerometer_statistics_item {
  float sum;
  float average;
  float variance;
  float maximum;

  float buffer[ACCELEROMETER_STATISTICS_BUFFER_SIZE];
  float buffer_max[ACCELEROMETER_STATISTICS_BUFFER_SIZE];
  size_t samples;
  size_t index;
};

struct koruza_motor_status {
  uint8_t connected;

  int32_t x;
  int32_t y;
  int32_t z;

  int32_t range_x;
  int32_t range_y;

  int32_t encoder_x;
  int32_t encoder_y;
};

struct koruza_camera_calibration {
  uint16_t port;
  char *path;
  uint32_t width;
  uint32_t height;
  uint32_t offset_x;
  uint32_t offset_y;
  uint32_t distance;
};

struct koruza_error_report {
  uint32_t code;
};

struct koruza_sfp_status {
  uint16_t tx_power;
  uint16_t rx_power;
};

struct koruza_accelerometer_status {
  uint8_t connected;

  struct accelerometer_statistics_item x[4];
  struct accelerometer_statistics_item y[4];
  struct accelerometer_statistics_item z[4];
};

struct koruza_status {
  char *serial_number;

  uint8_t gpio_reset;
  uint8_t leds;

  struct koruza_error_report errors;
  struct koruza_motor_status motors;
  struct koruza_accelerometer_status accelerometer;
  struct koruza_camera_calibration camera_calibration;
  struct koruza_sfp_status sfp;
};

struct survey_data_point {
  uint16_t rx_power;
};

struct koruza_survey {
  struct survey_data_point data[SURVEY_BINS][SURVEY_BINS];
};

int koruza_init(struct uci_context *uci, struct ubus_context *ubus);
int koruza_restore_motor();
int koruza_move_motor(int32_t x, int32_t y, int32_t z);
int koruza_homing();
int koruza_reboot();
int koruza_firmware_upgrade();
int koruza_hard_reset();
int koruza_update_status();
int koruza_set_webcam_calibration(uint32_t offset_x, uint32_t offset_y);
int koruza_set_distance(uint32_t distance);
void koruza_set_leds(uint8_t leds);
const struct koruza_status *koruza_get_status();

void koruza_survey_reset();
const struct koruza_survey *koruza_get_survey();

void koruza_compute_accelerometer_statistics();

#endif
