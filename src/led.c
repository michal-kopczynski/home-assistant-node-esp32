#include "led.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(led, LOG_LEVEL_INF);

#include <drivers/gpio.h>

const struct device * ledInit(const char *name) {
  int ret;

  const struct device *dev = device_get_binding(name);
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

void ledSet(const struct device * led, const bool state) {
  gpio_pin_set(led, PIN, (int)state);
}
