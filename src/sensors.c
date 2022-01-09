#include "sensors.h"
#include <logging/log.h>
LOG_MODULE_REGISTER(sensors, LOG_LEVEL_DBG);

#include <zephyr.h>

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

struct k_fifo sensorDataFifo;

struct SensorDataFifoItem {
    void *fifo_reserved;   /* 1st word reserved for use by FIFO */
    struct sensorsData data;
};

K_THREAD_STACK_DEFINE(sensorsThreadStackArea, STACKSIZE);
static struct k_thread sensorsThreadData;

static void sensorsThreadEntry()
{
  const char *tname;
  uint8_t cpu;
  struct k_thread *current_thread;
  int cnt = 0;

  LOG_DBG("SensorsThread started");
  while (1) {
    k_msleep(5000);

    current_thread = k_current_get();
    tname = k_thread_name_get(current_thread);

    static unsigned int cnt = 0;

    struct SensorDataFifoItem dataItem;

    dataItem.data.pms1_0 = 21;
    dataItem.data.pms2_5 = 22;
    dataItem.data.pms10 = 23;
    dataItem.data.temperature = cnt++;
    dataItem.data.humidity = 55;

    k_fifo_put(&sensorDataFifo, &dataItem);
  }
}

void initSensors() {
  k_fifo_init(&sensorDataFifo);

  k_thread_create(&sensorsThreadData, sensorsThreadStackArea,
                  K_THREAD_STACK_SIZEOF(sensorsThreadStackArea),
                  sensorsThreadEntry, NULL, NULL, NULL,
                  PRIORITY, 0, K_FOREVER);
  k_thread_name_set(&sensorsThreadData, "SensorsThread");

  k_thread_start(&sensorsThreadData);
}

bool getSensorsData(struct sensorsData* const data) {
  bool dataAvailable = !k_fifo_is_empty(&sensorDataFifo);

  if (dataAvailable) {
    LOG_DBG("Sensors data available");
    struct SensorDataFifoItem  *rx_data;
    rx_data = k_fifo_get(&sensorDataFifo, K_NO_WAIT);
    memcpy(data, &rx_data->data, sizeof(struct sensorsData));
  }

  return dataAvailable;
}
