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
#include <libubox/blobmsg.h>

#define MAX_SFP_MODULE_ID_LENGTH 64

// uBus context.
static struct ubus_context *koruza_ubus;
// Status of the connected KORUZA unit.
static struct koruza_status status;
// Timer for periodic status retrieval.
struct uloop_timeout timer_status;
// Timer for detection when MCU disconnects.
struct uloop_timeout timer_wait_reply;

int koruza_update_sfp();
void koruza_serial_message_handler(const message_t *message);
void koruza_timer_status_handler(struct uloop_timeout *timer);
void koruza_timer_wait_reply_handler(struct uloop_timeout *timer);

int koruza_init(struct uci_context *uci, struct ubus_context *ubus)
{
  koruza_ubus = ubus;

  memset(&status, 0, sizeof(struct koruza_status));
  serial_set_message_handler(koruza_serial_message_handler);

  // Setup timer handlers.
  timer_status.cb = koruza_timer_status_handler;
  timer_wait_reply.cb = koruza_timer_wait_reply_handler;
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
      uloop_timeout_cancel(&timer_wait_reply);

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
  // Send a status update request via the serial interface.
  message_t msg;
  message_init(&msg);
  message_tlv_add_command(&msg, COMMAND_GET_STATUS);
  message_tlv_add_checksum(&msg);
  serial_send_message(&msg);
  message_free(&msg);

  // Update data from the SFP driver.
  koruza_update_sfp();

  return 0;
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
    if (strcmp(bus, "/dev/i2c-0") == 0) {
      strncpy((char*) req->priv, module_id, MAX_SFP_MODULE_ID_LENGTH);
      break;
    }
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

  // TODO: Remove this test data.
  status.camera_calibration.offset_x = 200;
  status.camera_calibration.offset_y = 200;

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

void koruza_timer_status_handler(struct uloop_timeout *timeout)
{
  (void) timeout;

  koruza_update_status();

  // TODO: Make the periods configurable via UCI.
  uloop_timeout_set(&timer_status, 10000);
  uloop_timeout_set(&timer_wait_reply, 1000);
}

void koruza_timer_wait_reply_handler(struct uloop_timeout *timer)
{
  if (!status.connected) {
    return;
  }

  syslog(LOG_WARNING, "KORUZA MCU has been disconnected.");
  status.connected = 0;
}
