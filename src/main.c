#include "led.h"
#include "mqtt.h"
#include "sensors.h"
#include "wifi.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#include <zephyr.h>
#include <data/json.h>

#include <string.h>
#include <errno.h>

#define MQTT_PUB_TOPIC "nodes/outside/data"
#define MQTT_SUB_TOPIC "nodes/outside/set"

static char buffer[1024];

struct sensors_data
{
  const char *pms1_0;
  const char *pms2_5;
  const char *pms10;
  const char *temperature;
  const char *humidity;
};

struct node_data
{
  const char *location;
  const char *timestamp;
  const struct sensors_data sensors;
};

static const struct json_obj_descr node_data_sensor_descriptor[] = {
  JSON_OBJ_DESCR_PRIM(struct sensors_data, pms1_0, JSON_TOK_STRING),
  JSON_OBJ_DESCR_PRIM(struct sensors_data, pms2_5, JSON_TOK_STRING),
  JSON_OBJ_DESCR_PRIM(struct sensors_data, pms10, JSON_TOK_STRING),
  JSON_OBJ_DESCR_PRIM(struct sensors_data, temperature, JSON_TOK_STRING),
  JSON_OBJ_DESCR_PRIM(struct sensors_data, humidity, JSON_TOK_STRING),
};

static const struct json_obj_descr node_data_descriptor[] = {
  JSON_OBJ_DESCR_PRIM(struct node_data, location, JSON_TOK_STRING),
  JSON_OBJ_DESCR_PRIM(struct node_data, timestamp, JSON_TOK_STRING),
  JSON_OBJ_DESCR_OBJECT(struct node_data, sensors, node_data_sensor_descriptor),
};

char * encode_node_data(struct node_data response)
{
  int ret;
  ssize_t len;

  len = json_calc_encoded_len(node_data_descriptor, ARRAY_SIZE(node_data_descriptor), &response);

  ret = json_obj_encode_buf(
    node_data_descriptor,
    ARRAY_SIZE(node_data_descriptor),
    &response,
    buffer,
    sizeof(buffer));

  // LOG_INF("Encode Return Value: %d", ret);

  // TODO: handle error

  return buffer;
}

void main(void)
{
  const struct device * ledGreen = ledInit(LED0);
  if(ledGreen == NULL) {
    LOG_ERR("LED init failed!");
    return;
  }

  ledSet(ledGreen, false);

  wifiInit();

  mqttConnect();

  mqttSubscribe(MQTT_SUB_TOPIC);

  initSensors();

  while(1) {
    mqttProcess();

    struct sensorsData data;
    if (getSensorsData(&data)) {
      LOG_INF("Temperature: %u", data.temperature);
    }

    mqttPublish(MQTT_PUB_TOPIC, CONFIG_BOARD);

    ledSet(ledGreen, true);
    k_msleep(100);
    ledSet(ledGreen, false);
    k_msleep(900);
  }
}
