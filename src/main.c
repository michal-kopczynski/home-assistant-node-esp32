#include "mqtt.h"
#include "sensors.h"
#include "wifi.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#include <zephyr.h>
#include <data/json.h>

#include <string.h>
#include <errno.h>

#include <drivers/gpio.h>

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN	DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS	DT_GPIO_FLAGS(LED0_NODE, gpios)
#else
/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED0	""
#define PIN	0
#define FLAGS	0
#endif


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

static const struct device* configure_led_device() {
  int ret;

  const struct device *dev = device_get_binding(LED0);
  if (dev == NULL) {
    LOG_ERR("device_get_binding failed");
    return NULL;
  }

  ret = gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
  if (ret < 0) {
    LOG_ERR("gpio_pin_configure failed");
    return NULL;
  }

  return dev;
}

void main(void)
{
  const struct device * led = configure_led_device(&led);
  if(led == NULL) {
    LOG_ERR("Configuring LED failed!");
    return;
  }

  gpio_pin_set(led, PIN, (int)false);

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

    gpio_pin_set(led, PIN, (int)true);
    k_msleep(100);
    gpio_pin_set(led, PIN, (int)false);
    k_msleep(900);
  }
}
