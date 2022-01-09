#include "sensors.h"
#include <logging/log.h>
LOG_MODULE_REGISTER(wifi, LOG_LEVEL_DBG);

#include <net/socket.h>
#include <esp_wifi.h>

static struct net_mgmt_event_callback dhcpCb;
int nfds;

K_SEM_DEFINE(netIfReady, 0, 1);

static void netMgmtEventCb(struct net_mgmt_event_callback *cb,
        uint32_t mgmtEvent, struct net_if *iface)
{
  if (mgmtEvent != NET_EVENT_IPV4_DHCP_BOUND) {
    return;
  }
  k_sem_give(&netIfReady);
}

void wifiInit() {
  struct net_if *interface;

  net_mgmt_init_event_callback(&dhcpCb, netMgmtEventCb,
             NET_EVENT_IPV4_DHCP_BOUND);

  net_mgmt_add_event_callback(&dhcpCb);

  interface = net_if_get_default();
  if (!interface) {
    LOG_ERR("WiFi interface not available");
    return;
  }

  net_dhcpv4_start(interface);
  k_sem_take(&netIfReady, K_FOREVER);
}
