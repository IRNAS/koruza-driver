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
#include "ubus.h"
#include "koruza.h"
#include "network.h"
#include "upgrade.h"

#include <libubox/blobmsg.h>

// Ubus reply buffer.
static struct blob_buf reply_buf;

// Ubus attributes.
enum {
  KORUZA_MOTOR_X,
  KORUZA_MOTOR_Y,
  KORUZA_MOTOR_Z,
  __KORUZA_MOTOR_MAX,
};

static const struct blobmsg_policy koruza_motor_policy[__KORUZA_MOTOR_MAX] = {
  [KORUZA_MOTOR_X] = { .name = "x", .type = BLOBMSG_TYPE_INT32 },
  [KORUZA_MOTOR_Y] = { .name = "y", .type = BLOBMSG_TYPE_INT32 },
  [KORUZA_MOTOR_Z] = { .name = "z", .type = BLOBMSG_TYPE_INT32 },
};

static int ubus_move_motor(struct ubus_context *ctx, struct ubus_object *obj,
                           struct ubus_request_data *req, const char *method,
                           struct blob_attr *msg)
{
  struct blob_attr *tb[__KORUZA_MOTOR_MAX];

  blobmsg_parse(koruza_motor_policy, __KORUZA_MOTOR_MAX, tb, blob_data(msg), blob_len(msg));

  if (!tb[KORUZA_MOTOR_X] || !tb[KORUZA_MOTOR_Y] || !tb[KORUZA_MOTOR_Z]) {
    return UBUS_STATUS_INVALID_ARGUMENT;
  }

  int result = koruza_move_motor(
    (int32_t) blobmsg_get_u32(tb[KORUZA_MOTOR_X]),
    (int32_t) blobmsg_get_u32(tb[KORUZA_MOTOR_Y]),
    (int32_t) blobmsg_get_u32(tb[KORUZA_MOTOR_Z])
  );

  return result < 0 ? UBUS_STATUS_UNKNOWN_ERROR : UBUS_STATUS_OK;
}

static int ubus_get_status(struct ubus_context *ctx, struct ubus_object *obj,
                           struct ubus_request_data *req, const char *method,
                           struct blob_attr *msg)
{
  const struct koruza_status *status = koruza_get_status();
  const struct network_status *net_status = network_get_status();
  void *c;

  blob_buf_init(&reply_buf, 0);
  blobmsg_add_string(&reply_buf, "serial_number", status->serial_number);
  blobmsg_add_u8(&reply_buf, "connected", status->connected);

  c = blobmsg_open_table(&reply_buf, "leds");
  blobmsg_add_u8(&reply_buf, "state", status->leds);
  blobmsg_close_table(&reply_buf, c);

  c = blobmsg_open_table(&reply_buf, "errors");
  blobmsg_add_u32(&reply_buf, "code", status->errors.code);
  blobmsg_close_table(&reply_buf, c);

  c = blobmsg_open_table(&reply_buf, "motors");
  blobmsg_add_u32(&reply_buf, "x", status->motors.x);
  blobmsg_add_u32(&reply_buf, "y", status->motors.y);
  blobmsg_add_u32(&reply_buf, "z", status->motors.z);
  blobmsg_add_u32(&reply_buf, "range_x", status->motors.range_x);
  blobmsg_add_u32(&reply_buf, "range_y", status->motors.range_y);
  blobmsg_add_u32(&reply_buf, "encoder_x", status->motors.encoder_x);
  blobmsg_add_u32(&reply_buf, "encoder_y", status->motors.encoder_y);
  blobmsg_close_table(&reply_buf, c);

  c = blobmsg_open_table(&reply_buf, "camera_calibration");
  blobmsg_add_u16(&reply_buf, "port", status->camera_calibration.port);
  blobmsg_add_string(&reply_buf, "path", status->camera_calibration.path);
  blobmsg_add_u32(&reply_buf, "width", status->camera_calibration.width);
  blobmsg_add_u32(&reply_buf, "height", status->camera_calibration.height);
  blobmsg_add_u32(&reply_buf, "offset_x", status->camera_calibration.offset_x);
  blobmsg_add_u32(&reply_buf, "offset_y", status->camera_calibration.offset_y);
  blobmsg_add_u32(&reply_buf, "distance", status->camera_calibration.distance);
  blobmsg_close_table(&reply_buf, c);

  c = blobmsg_open_table(&reply_buf, "sfp");
  blobmsg_add_u16(&reply_buf, "tx_power", status->sfp.tx_power);
  blobmsg_add_u16(&reply_buf, "rx_power", status->sfp.rx_power);
  blobmsg_close_table(&reply_buf, c);

  c = blobmsg_open_table(&reply_buf, "network");
  blobmsg_add_string(&reply_buf, "interface", net_status->interface);
  blobmsg_add_string(&reply_buf, "ip_address", net_status->ip_address);
  blobmsg_add_u8(&reply_buf, "ready", net_status->ready);
  blobmsg_close_table(&reply_buf, c);

  ubus_send_reply(ctx, req, reply_buf.head);

  return UBUS_STATUS_OK;
}

static int ubus_homing(struct ubus_context *ctx, struct ubus_object *obj,
                       struct ubus_request_data *req, const char *method,
                       struct blob_attr *msg)
{
  return koruza_homing() < 0 ? UBUS_STATUS_UNKNOWN_ERROR : UBUS_STATUS_OK;
}

static int ubus_reboot(struct ubus_context *ctx, struct ubus_object *obj,
                       struct ubus_request_data *req, const char *method,
                       struct blob_attr *msg)
{
  return koruza_reboot() < 0 ? UBUS_STATUS_UNKNOWN_ERROR : UBUS_STATUS_OK;
}

static int ubus_firmware_upgrade(struct ubus_context *ctx, struct ubus_object *obj,
                                 struct ubus_request_data *req, const char *method,
                                 struct blob_attr *msg)
{
  return koruza_firmware_upgrade() < 0 ? UBUS_STATUS_UNKNOWN_ERROR : UBUS_STATUS_OK;
}

enum {
  KORUZA_CALIBRATION_X,
  KORUZA_CALIBRATION_Y,
  __KORUZA_CALIBRATION_MAX,
};

static const struct blobmsg_policy koruza_calibration_policy[__KORUZA_CALIBRATION_MAX] = {
  [KORUZA_CALIBRATION_X] = { .name = "x", .type = BLOBMSG_TYPE_INT32 },
  [KORUZA_CALIBRATION_Y] = { .name = "y", .type = BLOBMSG_TYPE_INT32 },
};

static int ubus_set_webcam_calibration(struct ubus_context *ctx, struct ubus_object *obj,
                                       struct ubus_request_data *req, const char *method,
                                       struct blob_attr *msg)
{
  struct blob_attr *tb[__KORUZA_CALIBRATION_MAX];

  blobmsg_parse(koruza_motor_policy, __KORUZA_CALIBRATION_MAX, tb, blob_data(msg), blob_len(msg));

  if (!tb[KORUZA_CALIBRATION_X] || !tb[KORUZA_CALIBRATION_Y]) {
    return UBUS_STATUS_INVALID_ARGUMENT;
  }

  int result = koruza_set_webcam_calibration(
    (int32_t) blobmsg_get_u32(tb[KORUZA_CALIBRATION_X]),
    (int32_t) blobmsg_get_u32(tb[KORUZA_CALIBRATION_Y])
  );

  return result < 0 ? UBUS_STATUS_UNKNOWN_ERROR : UBUS_STATUS_OK;
}

enum {
  KORUZA_DISTANCE_DISTANCE,
  __KORUZA_DISTANCE_MAX,
};

static const struct blobmsg_policy koruza_distance_policy[__KORUZA_DISTANCE_MAX] = {
  [KORUZA_DISTANCE_DISTANCE] = { .name = "distance", .type = BLOBMSG_TYPE_INT32 },
};

static int ubus_set_distance(struct ubus_context *ctx, struct ubus_object *obj,
                             struct ubus_request_data *req, const char *method,
                             struct blob_attr *msg)
{
  struct blob_attr *tb[__KORUZA_DISTANCE_MAX];

  blobmsg_parse(koruza_distance_policy, __KORUZA_DISTANCE_MAX, tb, blob_data(msg), blob_len(msg));

  if (!tb[KORUZA_DISTANCE_DISTANCE]) {
    return UBUS_STATUS_INVALID_ARGUMENT;
  }

  int result = koruza_set_distance(
    (int32_t) blobmsg_get_u32(tb[KORUZA_DISTANCE_DISTANCE])
  );

  return result < 0 ? UBUS_STATUS_UNKNOWN_ERROR : UBUS_STATUS_OK;
}

static int ubus_get_survey(struct ubus_context *ctx, struct ubus_object *obj,
                           struct ubus_request_data *req, const char *method,
                           struct blob_attr *msg)
{
  const struct koruza_survey *survey = koruza_get_survey();
  void *c;

  blob_buf_init(&reply_buf, 0);

  c = blobmsg_open_table(&reply_buf, "coverage");
  blobmsg_add_u16(&reply_buf, "x", SURVEY_COVERAGE);
  blobmsg_add_u16(&reply_buf, "y", SURVEY_COVERAGE);
  blobmsg_close_table(&reply_buf, c);

  c = blobmsg_open_table(&reply_buf, "bins");
  blobmsg_add_u16(&reply_buf, "x", SURVEY_BINS);
  blobmsg_add_u16(&reply_buf, "y", SURVEY_BINS);
  blobmsg_close_table(&reply_buf, c);

  c = blobmsg_open_array(&reply_buf, "data");
  for (size_t row = 0; row < SURVEY_BINS; row++) {
    void *c_row = blobmsg_open_array(&reply_buf, NULL);
    for (size_t col = 0; col < SURVEY_BINS; col++) {
      blobmsg_add_u16(&reply_buf, NULL, survey->data[row][col].rx_power);
    }
    blobmsg_close_array(&reply_buf, c_row);
  }
  blobmsg_close_array(&reply_buf, c);

  ubus_send_reply(ctx, req, reply_buf.head);

  return UBUS_STATUS_OK;
}

static int ubus_reset_survey(struct ubus_context *ctx, struct ubus_object *obj,
                             struct ubus_request_data *req, const char *method,
                             struct blob_attr *msg)
{
  koruza_survey_reset();

  return UBUS_STATUS_OK;
}

enum {
  KORUZA_LEDS_STATE,
  __KORUZA_LEDS_MAX,
};

static const struct blobmsg_policy koruza_leds_policy[__KORUZA_LEDS_MAX] = {
  [KORUZA_LEDS_STATE] = { .name = "state", .type = BLOBMSG_TYPE_INT8 },
};

static int ubus_set_leds(struct ubus_context *ctx, struct ubus_object *obj,
                         struct ubus_request_data *req, const char *method,
                         struct blob_attr *msg)
{
  struct blob_attr *tb[__KORUZA_LEDS_MAX];

  blobmsg_parse(koruza_leds_policy, __KORUZA_LEDS_MAX, tb, blob_data(msg), blob_len(msg));

  if (!tb[KORUZA_LEDS_STATE]) {
    return UBUS_STATUS_INVALID_ARGUMENT;
  }

  koruza_set_leds((int8_t) blobmsg_get_u8(tb[KORUZA_LEDS_STATE]));

  return UBUS_STATUS_OK;
}

static int ubus_upgrade(struct ubus_context *ctx, struct ubus_object *obj,
                        struct ubus_request_data *req, const char *method,
                        struct blob_attr *msg)
{
  return upgrade_start() < 0 ? UBUS_STATUS_UNKNOWN_ERROR : UBUS_STATUS_OK;
}

static const struct ubus_method koruza_methods[] = {
  UBUS_METHOD("move_motor", ubus_move_motor, koruza_motor_policy),
  UBUS_METHOD_NOARG("homing", ubus_homing),
  UBUS_METHOD_NOARG("reboot", ubus_reboot),
  UBUS_METHOD_NOARG("firmware_upgrade", ubus_firmware_upgrade),
  UBUS_METHOD_NOARG("get_status", ubus_get_status),
  UBUS_METHOD("set_webcam_calibration", ubus_set_webcam_calibration, koruza_calibration_policy),
  UBUS_METHOD("set_distance", ubus_set_distance, koruza_distance_policy),
  UBUS_METHOD_NOARG("get_survey", ubus_get_survey),
  UBUS_METHOD_NOARG("reset_survey", ubus_reset_survey),
  UBUS_METHOD("set_leds", ubus_set_leds, koruza_leds_policy),
  UBUS_METHOD_NOARG("upgrade", ubus_upgrade),
};

static struct ubus_object_type koruza_type =
  UBUS_OBJECT_TYPE("koruza", koruza_methods);

static struct ubus_object koruza_object = {
  .name = "koruza",
  .type = &koruza_type,
  .methods = koruza_methods,
  .n_methods = ARRAY_SIZE(koruza_methods),
};

int ubus_init(struct ubus_context *ubus)
{
  return ubus_add_object(ubus, &koruza_object);
}
