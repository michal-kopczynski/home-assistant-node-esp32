/*
 * Copyright (c) 2020 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <esp_wifi.h>
#include <esp_timer.h>
#include <esp_event.h>

#include <drivers/gpio.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>
#include <drivers/gpio.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(esp32_wifi_sta, LOG_LEVEL_DBG);


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


/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

static struct net_mgmt_event_callback dhcp_cb;

static void handler_cb(struct net_mgmt_event_callback *cb,
        uint32_t mgmt_event, struct net_if *iface)
{
  if (mgmt_event != NET_EVENT_IPV4_DHCP_BOUND) {
    return;
  }

  char buf[NET_IPV4_ADDR_LEN];

  LOG_INF("Your address: %s",
    log_strdup(net_addr_ntop(AF_INET,
           &iface->config.dhcpv4.requested_ip,
           buf, sizeof(buf))));
  LOG_INF("Lease time: %u seconds",
      iface->config.dhcpv4.lease_time);
  LOG_INF("Subnet: %s",
    log_strdup(net_addr_ntop(AF_INET,
          &iface->config.ip.ipv4->netmask,
          buf, sizeof(buf))));
  LOG_INF("Router: %s",
    log_strdup(net_addr_ntop(AF_INET,
            &iface->config.ip.ipv4->gw,
            buf, sizeof(buf))));
}

void main(void)
{
  const struct device *dev;
  bool led_is_on = true;
  int ret;

  dev = device_get_binding(LED0);
  if (dev == NULL) {
    return;
  }

  ret = gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
  if (ret < 0) {
    return;
  }

  LOG_INF("Hello ESP32 running Zephyr");



  struct net_if *iface;

  net_mgmt_init_event_callback(&dhcp_cb, handler_cb,
             NET_EVENT_IPV4_DHCP_BOUND);

  net_mgmt_add_event_callback(&dhcp_cb);

  iface = net_if_get_default();
  if (!iface) {
    LOG_ERR("wifi interface not available");
    return;
  }

  net_dhcpv4_start(iface);

  if (!IS_ENABLED(CONFIG_ESP32_WIFI_STA_AUTO)) {
    wifi_config_t wifi_config = {
      .sta = {
        .ssid = CONFIG_ESP32_WIFI_SSID,
        .password = CONFIG_ESP32_WIFI_PASSWORD,
      },
    };

    esp_err_t ret = esp_wifi_set_mode(WIFI_MODE_STA);

    ret |= esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    ret |= esp_wifi_connect();
    if (ret != ESP_OK) {
      LOG_ERR("connection failed");
    }
  }

  LOG_INF("Wifi end");

  while (1) {
    gpio_pin_set(dev, PIN, (int)led_is_on);
    led_is_on = !led_is_on;
    k_msleep(SLEEP_TIME_MS);

    LOG_INF("Blink");
  }
  LOG_INF("Main end");
}
