#include "led.h"
#include "mqtt.h"
#include "messages.h"
#include "sensors.h"
#include "wifi.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#include <zephyr.h>

#define MQTT_TOPIC_DATA "nodes/outside/data"
#define MQTT_TOPIC_SET "nodes/outside/set"

static char buffer[1024];

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

  mqttSubscribe(MQTT_TOPIC_SET);

  initSensors();

  while(1) {
    mqttProcess();

    struct SensorsData sensorsData;

    if (getSensorsData(&sensorsData)) {
      LOG_INF("Temperature: %u", sensorsData.temperature);

      const struct MessageNodeData nodeData = {
        .location = "outside",
        .timestamp = "2021-12-18T20:25:46.537Z",
        .sensors = {
          .pms1_0 = sensorsData.pms1_0,
          .pms2_5 = sensorsData.pms2_5,
          .pms10 = sensorsData.pms10,
          .temperature = sensorsData.temperature,
          .humidity = sensorsData.humidity,
        }
      };

      if (messageEncodeNodeData(&nodeData, buffer, sizeof(buffer))) {
        mqttPublish(MQTT_TOPIC_DATA, buffer);
      } else {
        LOG_ERR("Encode JSON failed!");
      }

      ledSet(ledGreen, true);
      k_msleep(100);
      ledSet(ledGreen, false);
    }

    k_msleep(1000);
  }
}
