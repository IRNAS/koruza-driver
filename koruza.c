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
#include "gpio.h"
#include "configuration.h"

#include "rpi_ws281x/ws2811.h"

#include <string.h>
#include <syslog.h>
#include <libubox/uloop.h>
#include <libubox/blobmsg.h>
#include <unistd.h>
#include <math.h>

#define MAX_SFP_MODULE_ID_LENGTH 64

#define KORUZA_SFP_REFRESH_INTERVAL 100
#define KORUZA_REFRESH_INTERVAL 500
#define KORUZA_MCU_TIMEOUT 2000
#define KORUZA_MCU_RESET_DELAY 120000
#define KORUZA_SURVEY_INTERVAL 700

#define LED_COUNT 25

// uBus context.
static struct ubus_context *koruza_ubus;
// UCI context.
static struct uci_context *koruza_uci;
// Status of the connected KORUZA unit.
static struct koruza_status status;
// Timer for periodic status retrieval.
struct uloop_timeout timer_status;
// Timer for periodic SFP status retrieval.
struct uloop_timeout timer_sfp_status;
// Timer for periodic survey updates.
struct uloop_timeout timer_survey;
// Timer for detection when MCU disconnects.
struct uloop_timeout timer_wait_reply;
// Survey.
static struct koruza_survey survey;

// LED configuration.
static ws2811_t led_config = {
  .freq = WS2811_TARGET_FREQ,
  .dmanum = 5,
  .channel =
  {
    [0] =
    {
      .gpionum = 40,
      .count = LED_COUNT,
      .invert = 0,
      .brightness = 255,
      .strip_type = WS2811_STRIP_GRB,
    },
    [1] =
    {
      .gpionum = 0,
      .count = 0,
      .invert = 0,
      .brightness = 0,
    },
  },
};

struct color_map {
  int power;
  ws2811_led_t color;
};

static struct color_map led_color_map[] = {
  {.power = -40, .color = 0x00FF0000},
  {.power = -38, .color = 0x00FF4500},
  {.power = -30, .color = 0x00FF7F50},
  {.power = -25, .color = 0x00FF00FF},
  {.power = -20, .color = 0x000000FF},
  {.power = -15, .color = 0x000045FF},
  {.power = -10, .color = 0x0000FFFF},
  {.power =  -5, .color = 0x0000FF00},
};

int koruza_update_sfp();
int koruza_update_sfp_leds();
int koruza_uci_commit();
void koruza_serial_motors_message_handler(const message_t *message);
void koruza_serial_accelerometer_message_handler(const message_t *message);
void koruza_timer_status_handler(struct uloop_timeout *timer);
void koruza_timer_sfp_status_handler(struct uloop_timeout *timer);
void koruza_timer_wait_reply_handler(struct uloop_timeout *timer);
void koruza_timer_survey_handler(struct uloop_timeout *timer);

int koruza_init(struct uci_context *uci, struct ubus_context *ubus)
{
  koruza_ubus = ubus;
  koruza_uci = uci;

  memset(&status, 0, sizeof(struct koruza_status));
  serial_set_message_handler(DEVICE_MOTORS, koruza_serial_motors_message_handler);
  serial_set_message_handler(DEVICE_ACCELEROMETER, koruza_serial_accelerometer_message_handler);

  koruza_survey_reset();

  // Configure serial number or default to '0000' if not configured.
  status.serial_number = uci_get_string(uci, "koruza.@unit[0].serial_number");
  if (status.serial_number == NULL) {
    status.serial_number = "0000";
  }

  // Initialize calibration defaults.
  status.camera_calibration.port = uci_get_int(uci, "koruza.@webcam[0].port", 8080);
  status.camera_calibration.path = uci_get_string(uci, "koruza.@webcam[0].path");
  status.camera_calibration.width = uci_get_int(uci, "koruza.@webcam[0].width", 1280);
  status.camera_calibration.height = uci_get_int(uci, "koruza.@webcam[0].height", 720);

  char *webcam_resolution = uci_get_string(uci, "mjpg-streamer.core.resolution");
  if (webcam_resolution != NULL) {
    sscanf(
      webcam_resolution,
      "%dx%d",
      &status.camera_calibration.width,
      &status.camera_calibration.height
    );

    free(webcam_resolution);
  }

  status.camera_calibration.offset_x = uci_get_int(uci, "koruza.@webcam[0].offset_x", 0);
  status.camera_calibration.offset_y = uci_get_int(uci, "koruza.@webcam[0].offset_y", 0);
  status.camera_calibration.distance = uci_get_int(uci, "koruza.@webcam[0].distance", 0);

  status.motors.range_x = uci_get_int(uci, "koruza.@motors[0].range_x", 25000);
  if (status.motors.range_x <= 0) {
    syslog(LOG_ERR, "Invalid range specified for X direction, defaulting to 25000.");
    status.motors.range_x = 25000;
  }

  status.motors.range_y = uci_get_int(uci, "koruza.@motors[0].range_y", 25000);
  if (status.motors.range_y <= 0) {
    syslog(LOG_ERR, "Invalid range specified for Y direction, defaulting to 25000.");
    status.motors.range_y = 25000;
  }

  status.motors.x = uci_get_int(uci, "koruza.@motors[0].last_x", 0);
  if (status.motors.x < -status.motors.range_x || status.motors.x > status.motors.range_x) {
    status.motors.x = 0;
  }

  status.motors.y = uci_get_int(uci, "koruza.@motors[0].last_y", 0);
  if (status.motors.y < -status.motors.range_y || status.motors.y > status.motors.range_y) {
    status.motors.y = 0;
  }

  // Setup timer handlers.
  timer_status.cb = koruza_timer_status_handler;
  timer_sfp_status.cb = koruza_timer_sfp_status_handler;
  timer_wait_reply.cb = koruza_timer_wait_reply_handler;
  timer_survey.cb = koruza_timer_survey_handler;
  uloop_timeout_set(&timer_status, KORUZA_REFRESH_INTERVAL);
  uloop_timeout_set(&timer_sfp_status, KORUZA_SFP_REFRESH_INTERVAL);
  uloop_timeout_set(&timer_survey, KORUZA_SURVEY_INTERVAL);

  // Initialize LEDs.
  status.leds = uci_get_int(uci, "koruza.leds.status", 1);
  led_config.channel[0].gpionum = uci_get_int(uci, "koruza.leds.gpio", 40);
  if (ws2811_init(&led_config) != WS2811_SUCCESS) {
    syslog(LOG_WARNING, "Failed to initialize LEDs.");
  } else {
    koruza_update_sfp_leds();
  }

  // Perform a hard MCU reset.
  status.gpio_reset = uci_get_int(uci, "koruza.@mcu[0].gpio_reset", 18);
  if (koruza_hard_reset() != 0) {
    syslog(LOG_WARNING, "Failed to trigger MCU reset.");
  }

  return koruza_update_status();
}

const struct koruza_status *koruza_get_status()
{
  return &status;
}

const struct koruza_survey *koruza_get_survey()
{
  return &survey;
}

void koruza_serial_motors_message_handler(const message_t *message)
{
  // Check if this is a reply or a command message.
  tlv_reply_t reply = 0;
  tlv_command_t command = 0;
  message_tlv_get_reply(message, &reply);
  message_tlv_get_command(message, &command);
  if (!reply && !command) {
    return;
  }

  switch (reply) {
    case REPLY_STATUS_REPORT: {
      uloop_timeout_cancel(&timer_wait_reply);

      if (!status.motors.connected) {
        // Was not considered connected until now.
        syslog(LOG_INFO, "Detected KORUZA motor driver on the configured serial port.");
        status.motors.connected = 1;

        // Restore motor position.
        koruza_restore_motor();
      }

      // Handle motor position report.
      tlv_motor_position_t position;
      if (message_tlv_get_motor_position(message, &position) == MESSAGE_SUCCESS) {
        status.motors.x = position.x;
        status.motors.y = position.y;
        status.motors.z = position.z;

        // Save stored position (when in range).
        if (status.motors.x >= -status.motors.range_x && status.motors.x <= status.motors.range_x &&
            status.motors.y >= -status.motors.range_y && status.motors.y <= status.motors.range_y) {
          uci_set_int(koruza_uci, "koruza.@motors[0].last_x", status.motors.x);
          uci_set_int(koruza_uci, "koruza.@motors[0].last_y", status.motors.y);

          // Commit changes.
          koruza_uci_commit();
        } else {
          syslog(LOG_WARNING, "MCU sent an out-of-range motor position.");
        }
      }

      // Handle encoder value report.
      tlv_encoder_value_t encoder_value;
      if (message_tlv_get_encoder_value(message, &encoder_value) == MESSAGE_SUCCESS) {
        status.motors.encoder_x = encoder_value.x;
        status.motors.encoder_y = encoder_value.y;
      }

      break;
    }

    case REPLY_ERROR_REPORT: {
      // Parse the error report.
      tlv_error_report_t error;
      if (message_tlv_get_error_report(message, &error) == MESSAGE_SUCCESS) {
        status.errors.code = error.code;
      }

      break;
    }
  }

  switch (command) {
    case COMMAND_RESTORE_MOTOR: {
      // Explicit request from the MCU to restore motor position.
      koruza_restore_motor();
      break;
    }

    default: {
      // Ignore.
    }
  }
}

void koruza_serial_accelerometer_message_handler(const message_t *message)
{
  // Check if this is a reply or a command message.
  tlv_reply_t reply = 0;
  tlv_command_t command = 0;
  message_tlv_get_reply(message, &reply);
  message_tlv_get_command(message, &command);
  if (!reply && !command) {
    return;
  }

  switch (reply) {
    case REPLY_STATUS_REPORT: {
      if (!status.accelerometer.connected) {
        // Was not considered connected until now.
        syslog(LOG_INFO, "Detected accelerometer driver on the configured serial port.");
        status.accelerometer.connected = 1;
      }

      // Handle accelerometer value report.
      tlv_accelerometer_value_t accelerometer_value;
      if (message_tlv_get_accelerometer_value(message, &accelerometer_value) == MESSAGE_SUCCESS) {
        status.accelerometer.ax = accelerometer_value.ax;
        status.accelerometer.ay = accelerometer_value.ay;
        status.accelerometer.az = accelerometer_value.az;
      }

      break;
    }

    default: {
      // Ignore.
    }
  }
}

int koruza_restore_motor()
{
  tlv_motor_position_t position;
  position.x = status.motors.x;
  position.y = status.motors.y;
  position.z = status.motors.z;

  message_t msg;
  message_init(&msg);
  message_tlv_add_command(&msg, COMMAND_RESTORE_MOTOR);
  message_tlv_add_motor_position(&msg, &position);
  message_tlv_add_checksum(&msg);
  serial_send_message(DEVICE_MOTORS, &msg);
  message_free(&msg);
  return 0;
}

int koruza_move_motor(int32_t x, int32_t y, int32_t z)
{
  if (!status.motors.connected) {
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
  serial_send_message(DEVICE_MOTORS, &msg);
  message_free(&msg);

  return 0;
}

int koruza_homing()
{
  if (!status.motors.connected) {
    return -1;
  }

  message_t msg;
  message_init(&msg);
  message_tlv_add_command(&msg, COMMAND_HOMING);
  message_tlv_add_checksum(&msg);
  serial_send_message(DEVICE_MOTORS, &msg);
  message_free(&msg);

  return 0;
}

int koruza_reboot()
{
  message_t msg;
  message_init(&msg);
  message_tlv_add_command(&msg, COMMAND_REBOOT);
  message_tlv_add_checksum(&msg);
  serial_send_message(DEVICE_MOTORS, &msg);
  message_free(&msg);

  return 0;
}

int koruza_firmware_upgrade()
{
  message_t msg;
  message_init(&msg);
  message_tlv_add_command(&msg, COMMAND_FIRMWARE_UPGRADE);
  message_tlv_add_checksum(&msg);
  serial_send_message(DEVICE_MOTORS, &msg);
  message_free(&msg);

  return 0;
}

int koruza_hard_reset()
{
  if (gpio_export(status.gpio_reset) != 0) {
    return -1;
  }

  if (gpio_direction(status.gpio_reset, GPIO_OUT) != 0) {
    gpio_unexport(status.gpio_reset);
    return -1;
  }

  if (gpio_write(status.gpio_reset, GPIO_HIGH) != 0) {
    gpio_unexport(status.gpio_reset);
    return -1;
  }

  usleep(KORUZA_MCU_RESET_DELAY);

  if (gpio_write(status.gpio_reset, GPIO_LOW) != 0) {
    gpio_unexport(status.gpio_reset);
    return -1;
  }

  gpio_unexport(status.gpio_reset);
  return 0;
}

int koruza_update_sfp_leds()
{
  double rx_power_dbm = 10.0 * log10(((double) status.sfp.rx_power) / 10000.0);
  if (rx_power_dbm < -40.0) {
    rx_power_dbm = -40.0;
  }

  // Update LEDs based on SFP power.
  ws2811_led_t color = led_color_map[0].color;
  for (size_t i = 0; i < sizeof(led_color_map) / sizeof(struct color_map); i++) {
    if (rx_power_dbm >= (double) led_color_map[i].power) {
      color = led_color_map[i].color;
    }
  }

  for (size_t i = 0; i < LED_COUNT; i++) {
    led_config.channel[0].leds[i] = color;
  }

  // Set brightness based on LED state.
  if (status.leds) {
    led_config.channel[0].brightness = 255;
  } else {
    led_config.channel[0].brightness = 0;
  }

  if (ws2811_render(&led_config) != WS2811_SUCCESS) {
    syslog(LOG_WARNING, "Failed to render LED status.");
    return -1;
  }

  return 0;
}

int koruza_update_status()
{
  // Update data from the SFP driver.
  koruza_update_sfp();
  koruza_update_sfp_leds();

  // Send a status update request via the serial interface.
  message_t msg;
  message_init(&msg);
  message_tlv_add_command(&msg, COMMAND_GET_STATUS);
  message_tlv_add_power_reading(&msg, status.sfp.rx_power);
  message_tlv_add_checksum(&msg);

  if (serial_send_message(DEVICE_MOTORS, &msg) != 0) {
    status.motors.connected = 0;
  }

  if (serial_send_message(DEVICE_ACCELEROMETER, &msg) != 0) {
    status.accelerometer.connected = 0;
  }

  message_free(&msg);

  return 0;
}

int koruza_uci_commit()
{
  struct uci_ptr ptr;
  if (uci_lookup_ptr(koruza_uci, &ptr, "koruza", true) != UCI_OK ||
      uci_commit(koruza_uci, &ptr.p, false) != UCI_OK) {
    syslog(LOG_ERR, "Failed to commit updated webcam calibration offsets.");
    return 1;
  }

  return 0;
}

int koruza_set_webcam_calibration(uint32_t offset_x, uint32_t offset_y)
{
  status.camera_calibration.offset_x = offset_x;
  status.camera_calibration.offset_y = offset_y;

  uci_set_int(koruza_uci, "koruza.@webcam[0].offset_x", offset_x);
  uci_set_int(koruza_uci, "koruza.@webcam[0].offset_y", offset_y);

  return koruza_uci_commit();
}

int koruza_set_distance(uint32_t distance)
{
  status.camera_calibration.distance = distance;

  uci_set_int(koruza_uci, "koruza.@webcam[0].distance", distance);

  return koruza_uci_commit();
}

enum {
  SFP_GET_MODULES_BUS,
  __SFP_GET_MODULES_MAX,
};

static const struct blobmsg_policy sfp_get_modules_policy[__SFP_GET_MODULES_MAX] = {
  [SFP_GET_MODULES_BUS] = { .name = "bus", .type = BLOBMSG_TYPE_STRING },
};

static void koruza_sfp_get_module(struct ubus_request *req, int type, struct blob_attr *msg)
{
  struct blob_attr *module;
  int rem;

  blobmsg_for_each_attr(module, msg, rem) {
    const char *module_id = blobmsg_name(module);
    struct blob_attr *tb[__SFP_GET_MODULES_MAX];

    blobmsg_parse(sfp_get_modules_policy, __SFP_GET_MODULES_MAX, tb, blobmsg_data(module), blobmsg_data_len(module));

    if (!tb[SFP_GET_MODULES_BUS]) {
      continue;
    }

    const char *bus = blobmsg_get_string(tb[SFP_GET_MODULES_BUS]);
    // TODO: Better way to detect primary module.
    if (strcmp(bus, "/dev/i2c-1") == 0) {
      strncpy((char*) req->priv, module_id, MAX_SFP_MODULE_ID_LENGTH);
      break;
    }
  }
}

enum {
  SFP_GET_DIAG_VALUE,
  __SFP_GET_DIAG_MAX,
};

static const struct blobmsg_policy sfp_get_diagnostics_policy[__SFP_GET_DIAG_MAX] = {
  [SFP_GET_DIAG_VALUE] = { .name = "value", .type = BLOBMSG_TYPE_TABLE },
};

enum {
  SFP_DIAG_ITEM_TX_POWER,
  SFP_DIAG_ITEM_RX_POWER,
  __SFP_DIAG_ITEM_MAX,
};

static const struct blobmsg_policy sfp_diagnostics_item_policy[__SFP_DIAG_ITEM_MAX] = {
  [SFP_DIAG_ITEM_TX_POWER] = { .name = "tx_power", .type = BLOBMSG_TYPE_STRING },
  [SFP_DIAG_ITEM_RX_POWER] = { .name = "rx_power", .type = BLOBMSG_TYPE_STRING },
};

static void koruza_sfp_get_diagnostics(struct ubus_request *req, int type, struct blob_attr *msg)
{
  struct blob_attr *module;
  int rem;

  blobmsg_for_each_attr(module, msg, rem) {
    struct blob_attr *tb[__SFP_GET_DIAG_MAX];
    struct blob_attr *tb_value[__SFP_DIAG_ITEM_MAX];

    blobmsg_parse(sfp_get_diagnostics_policy, __SFP_GET_DIAG_MAX, tb,
      blobmsg_data(module), blobmsg_data_len(module));

    if (!tb[SFP_GET_DIAG_VALUE]) {
      continue;
    }

    blobmsg_parse(sfp_diagnostics_item_policy, __SFP_DIAG_ITEM_MAX, tb_value,
      blobmsg_data(tb[SFP_GET_DIAG_VALUE]), blobmsg_data_len(tb[SFP_GET_DIAG_VALUE]));

    if (tb_value[SFP_DIAG_ITEM_TX_POWER]) {
      const char *tx_power = blobmsg_get_string(tb_value[SFP_DIAG_ITEM_TX_POWER]);
      float tx_power_float = 0;
      sscanf(tx_power, "%f", &tx_power_float);
      status.sfp.tx_power = (uint16_t) (tx_power_float * 10000);
    }

    if (tb_value[SFP_DIAG_ITEM_RX_POWER]) {
      const char *rx_power = blobmsg_get_string(tb_value[SFP_DIAG_ITEM_RX_POWER]);
      float rx_power_float = 0;
      sscanf(rx_power, "%f", &rx_power_float);
      status.sfp.rx_power = (uint16_t) (rx_power_float * 10000);
    }

    // Only process the first module.
    break;
  }
}

enum {
  SFP_VENDOR_DATA,
  __SFP_VENDOR_MAX,
};

static const struct blobmsg_policy sfp_vendor_policy[__SFP_VENDOR_MAX] = {
  [SFP_VENDOR_DATA] = { .name = "vendor_specific", .type = BLOBMSG_TYPE_UNSPEC },
};

static void koruza_sfp_get_calibration_data(struct ubus_request *req, int type, struct blob_attr *msg)
{
  struct blob_attr *tb[__SFP_VENDOR_MAX];

  blobmsg_parse(sfp_vendor_policy, __SFP_VENDOR_MAX, tb, blob_data(msg), blob_len(msg));
  if (!tb[SFP_VENDOR_DATA]) {
    return;
  }

  uint8_t *vendor_specific = (uint8_t*) blobmsg_data(tb[SFP_VENDOR_DATA]);
  size_t vendor_specific_length = blobmsg_data_len(tb[SFP_VENDOR_DATA]);

  // Assume the calibration data contains TLVs.
  message_t calibration_msg;
  tlv_sfp_calibration_t calibration;
  message_init(&calibration_msg);
  if (message_parse(&calibration_msg, vendor_specific, vendor_specific_length) != MESSAGE_SUCCESS) {
    return;
  }

  if (message_tlv_get_sfp_calibration(&calibration_msg, &calibration) != MESSAGE_SUCCESS) {
    message_free(&calibration_msg);
    return;
  }

  status.camera_calibration.offset_x = calibration.offset_x;
  status.camera_calibration.offset_y = calibration.offset_y;

  message_free(&calibration_msg);
}

int koruza_update_sfp()
{
  uint32_t ubus_id;
  if (ubus_lookup_id(koruza_ubus, "sfp", &ubus_id)) {
    // The SFP driver does not seem to be running.
    return -1;
  }

  static struct blob_buf req;

  // Fetch a list of modules and get the first identifier of the module on bus /dev/i2c-0.
  char module_id[MAX_SFP_MODULE_ID_LENGTH] = {0,};
  blob_buf_init(&req, 0);
  if (ubus_invoke(
        koruza_ubus,
        ubus_id,
        "get_modules",
        req.head,
        koruza_sfp_get_module,
        &module_id,
        1000
      ) != UBUS_STATUS_OK) {
    return -1;
  }

  // Get diagnostic data for this module.
  blob_buf_init(&req, 0);
  blobmsg_add_string(&req, "module", module_id);
  if (ubus_invoke(
        koruza_ubus,
        ubus_id,
        "get_diagnostics",
        req.head,
        koruza_sfp_get_diagnostics,
        NULL,
        1000
      ) != UBUS_STATUS_OK) {
    return -1;
  }

  // Now get vendor-specific data for this module.
  blob_buf_init(&req, 0);
  blobmsg_add_string(&req, "module", module_id);
  if (ubus_invoke(
        koruza_ubus,
        ubus_id,
        "get_vendor_specific_data",
        req.head,
        koruza_sfp_get_calibration_data,
        NULL,
        1000
      ) != UBUS_STATUS_OK) {
    return -1;
  }

  return 0;
}

void koruza_timer_status_handler(struct uloop_timeout *timer)
{
  (void) timer;

  koruza_update_status();

  uloop_timeout_set(&timer_status, KORUZA_REFRESH_INTERVAL);

  if (!timer_wait_reply.pending)
    uloop_timeout_set(&timer_wait_reply, KORUZA_MCU_TIMEOUT);
}

void koruza_timer_sfp_status_handler(struct uloop_timeout *timer)
{
  // Update data from the SFP driver.
  koruza_update_sfp();
  koruza_update_sfp_leds();

  uloop_timeout_set(timer, KORUZA_SFP_REFRESH_INTERVAL);
}

void koruza_timer_wait_reply_handler(struct uloop_timeout *timer)
{
  if (!status.motors.connected) {
    return;
  }

  syslog(LOG_WARNING, "KORUZA motor driver has been disconnected.");
  status.motors.connected = 0;
}

void koruza_survey_reset()
{
  memset(&survey, 0, sizeof(survey));
}

void koruza_timer_survey_handler(struct uloop_timeout *timer)
{
  uloop_timeout_set(timer, KORUZA_SURVEY_INTERVAL);

  if (!status.motors.connected) {
    return;
  }

  int x_bin = (status.motors.x * (SURVEY_BINS / 2)) / SURVEY_COVERAGE + (SURVEY_BINS / 2);
  int y_bin = (status.motors.y * (SURVEY_BINS / 2)) / SURVEY_COVERAGE + (SURVEY_BINS / 2);

  if (x_bin < 0) x_bin = 0;
  if (x_bin >= SURVEY_BINS) x_bin = SURVEY_BINS - 1;
  if (y_bin < 0) y_bin = 0;
  if (y_bin >= SURVEY_BINS) y_bin = SURVEY_BINS - 1;

  survey.data[y_bin][x_bin].rx_power = status.sfp.rx_power;
}

void koruza_set_leds(uint8_t leds)
{
  status.leds = leds;

  // Persist LED configuration.
  uci_set_string(koruza_uci, "koruza.leds", "leds");
  uci_set_int(koruza_uci, "koruza.leds.status", leds);
  koruza_uci_commit();

  koruza_update_sfp_leds();
}
