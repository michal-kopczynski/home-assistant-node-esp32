#include "messages.h"
#include <logging/log.h>
LOG_MODULE_REGISTER(protocol, LOG_LEVEL_DBG);

#include <data/json.h>

static const struct json_obj_descr messageSensorsDataDescriptor[] = {
  JSON_OBJ_DESCR_PRIM(struct MessageSensorsData, pms1_0, JSON_TOK_NUMBER),
  JSON_OBJ_DESCR_PRIM(struct MessageSensorsData, pms2_5, JSON_TOK_NUMBER),
  JSON_OBJ_DESCR_PRIM(struct MessageSensorsData, pms10, JSON_TOK_NUMBER),
  JSON_OBJ_DESCR_PRIM(struct MessageSensorsData, temperature, JSON_TOK_NUMBER),
  JSON_OBJ_DESCR_PRIM(struct MessageSensorsData, humidity, JSON_TOK_NUMBER),
};

static const struct json_obj_descr messageDataDescriptor[] = {
  JSON_OBJ_DESCR_PRIM(struct MessageNodeData, location, JSON_TOK_STRING),
  JSON_OBJ_DESCR_PRIM(struct MessageNodeData, timestamp, JSON_TOK_STRING),
  JSON_OBJ_DESCR_OBJECT(struct MessageNodeData, sensors, messageSensorsDataDescriptor),
};

bool messageEncodeNodeData(const struct MessageNodeData* response, char * const buffer, const size_t bufferSize)
{
  int ret;
  ssize_t len = json_calc_encoded_len(messageDataDescriptor, ARRAY_SIZE(messageDataDescriptor), response);
  LOG_DBG("Required buffer length: %u", len);

  return json_obj_encode_buf(
    messageDataDescriptor,
    ARRAY_SIZE(messageDataDescriptor),
    response,
    buffer,
    bufferSize) == NULL;
}
