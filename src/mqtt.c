#include "mqtt.h"
#include <logging/log.h>
LOG_MODULE_REGISTER(mqtt, LOG_LEVEL_DBG);

#include <random/rand32.h>
#include <net/socket.h>
#include <net/mqtt.h>
#include <esp_event.h>


static uint8_t rx_buffer[1024];
static uint8_t tx_buffer[1024];
static struct mqtt_client mqttClient;
struct mqtt_utf8 app_user_name;
struct mqtt_utf8 app_password;

static struct sockaddr_storage broker;
static struct zsock_pollfd fds[1];
extern int nfds;

#define MQTT_POLL_MSEC	500
#define MQTT_BROKER_IP "192.168.1.4"
#define MQTT_USER ""
#define MQTT_PWD ""

static void prepare_fds(struct mqtt_client *client) {
  if (client->transport.type == MQTT_TRANSPORT_NON_SECURE) {
    fds[0].fd = client->transport.tcp.sock;
  }

  fds[0].events = ZSOCK_POLLIN;
  nfds = 1;
}

static int poll_socks(int timeout) {
  int ret = 0;

  if (nfds > 0) {
    ret = zsock_poll(fds, nfds, timeout);
    if (ret < 0) {
      LOG_ERR("poll error: %d", errno);
    }
  }

  return ret;
}

static void app_mqtt_evt_handler(struct mqtt_client *const client,
          const struct mqtt_evt *evt) {
  struct mqtt_puback_param puback;
  uint8_t data[128];
  int len;
  int bytes_read;
  int err;

  switch (evt->type) {
    case MQTT_EVT_CONNACK:  {
      if (evt->result != 0) {
        LOG_ERR("MQTT connect failed %d", evt->result);
        break;
      }
      LOG_INF("MQTT client connected!");

      break;
    }

    case MQTT_EVT_PUBLISH: {
      len = evt->param.publish.message.payload.len;

      LOG_INF("MQTT publish received %d, %d bytes", evt->result, len);
      LOG_INF(" id: %d, qos: %d", evt->param.publish.message_id,
        evt->param.publish.message.topic.qos);

      while (len) {
        bytes_read = mqtt_read_publish_payload(&mqttClient,
            data,
            len >= sizeof(data) - 1 ?
            sizeof(data) - 1 : len);
        if (bytes_read < 0 && bytes_read != -EAGAIN) {
          LOG_ERR("failure to read payload");
          break;
        }

        data[bytes_read] = '\0';
        LOG_INF("   payload: %s", log_strdup(data));
        len -= bytes_read;
      }

      puback.message_id = evt->param.publish.message_id;
      mqtt_publish_qos1_ack(&mqttClient, &puback);
      break;
    }

    case MQTT_EVT_DISCONNECT: {
      LOG_INF("MQTT client disconnected %d", evt->result);
      break;
    }

    case MQTT_EVT_PUBACK: {
      if (evt->result != 0) {
        LOG_ERR("MQTT PUBACK error %d", evt->result);
        break;
      }
      LOG_INF("PUBACK packet id: %u", evt->param.puback.message_id);
      break;
    }

    case MQTT_EVT_PUBREC: {
      if (evt->result != 0) {
        LOG_ERR("MQTT PUBREC error %d", evt->result);
        break;
      }

      LOG_INF("PUBREC packet id: %u", evt->param.pubrec.message_id);

      const struct mqtt_pubrel_param rel_param = {
        .message_id = evt->param.pubrec.message_id
      };

      err = mqtt_publish_qos2_release(client, &rel_param);
      if (err != 0) {
        LOG_ERR("Failed to send MQTT PUBREL: %d", err);
      }

      break;
    }

    case MQTT_EVT_PUBCOMP: {
      if (evt->result != 0) {
        LOG_ERR("MQTT PUBCOMP error %d", evt->result);
        break;
      }

      LOG_INF("PUBCOMP packet id: %u",
        evt->param.pubcomp.message_id);

      break;
    }

    case MQTT_EVT_SUBACK: {
      LOG_INF("SUBACK packet id: %u", evt->param.suback.message_id);
      break;
    }

    case MQTT_EVT_UNSUBACK: {
      LOG_INF("UNSUBACK packet id: %u", evt->param.suback.message_id);
      break;
    }

    case MQTT_EVT_PINGRESP: {
      LOG_INF("PINGRESP packet");
      break;
    }

    default: {
      LOG_INF("Unhandled MQTT event %d", evt->type);
      break;
    }
  }
}

static void app_mqtt_broker_init(void) {
  struct sockaddr_in *addr = (struct sockaddr_in *)&broker;

  addr->sin_family = AF_INET;
  addr->sin_port = htons(1883);

  zsock_inet_pton(AF_INET, MQTT_BROKER_IP, &addr->sin_addr);
}

static void mqttClientInit(struct mqtt_client *client) {
  mqtt_client_init(client);

  app_mqtt_broker_init();

  app_user_name.utf8 = (const uint8_t *)MQTT_USER;
  app_user_name.size = strlen(app_user_name.utf8);

  app_password.utf8 = (const uint8_t *) MQTT_PWD;
  app_password.size = strlen(app_password.utf8);

  /* MQTT client configuration */
  client->broker = &broker;
  client->evt_cb = app_mqtt_evt_handler;
  client->client_id.utf8 = (const uint8_t *)"nodes/outside";
  client->client_id.size = strlen(client->client_id.utf8);

  client->password = &app_password;
  client->user_name = &app_user_name;
  client->protocol_version = MQTT_VERSION_3_1_1;

  /* MQTT buffers configuration */
  client->rx_buf = rx_buffer;
  client->rx_buf_size = sizeof(rx_buffer);
  client->tx_buf = tx_buffer;
  client->tx_buf_size = sizeof(tx_buffer);

  /* MQTT transport configuration */
  client->transport.type = MQTT_TRANSPORT_NON_SECURE;
}


int mqttConnect() {
  int rc, i = 0;

  mqttClientInit(&mqttClient);

  rc = mqtt_connect(&mqttClient);
  if (rc != 0) {
    return rc;
  }

  prepare_fds(&mqttClient);

  if (poll_socks(MQTT_POLL_MSEC)) {
    mqtt_input(&mqttClient);
  }

  return rc;
}

void mqttProcess() {
  int rc;

  if (poll_socks(MQTT_POLL_MSEC)) {
    rc = mqtt_input(&mqttClient);
    if (rc != 0) {
      return;
    }
  }

  rc = mqtt_live(&mqttClient);
  if (rc != 0 && rc != -EAGAIN) {
    return;
  } else if (rc == 0) {
    rc = mqtt_input(&mqttClient);
    if (rc != 0) {
      return;
    }
  }

  return;
}

void mqttSubscribe(const char* const topicName) {
  int rc;
  struct mqtt_topic topic = {};
  struct mqtt_subscription_list subs_list = {};

  LOG_INF("Subscribe for topic %s", topicName);

  topic.topic.utf8 = (uint8_t *)topicName;
  topic.topic.size = strlen(topic.topic.utf8);
  subs_list.list = &topic;
  subs_list.list_count = 1;
  subs_list.message_id = 1;

  rc = mqtt_subscribe(&mqttClient, &subs_list);
  if (rc) {
    LOG_ERR("Subscribe failed for topic %s", topicName);
  }
}

void mqttPublish(const char* const topicName, char* message) {
  struct mqtt_publish_param param;

  param.message.topic.qos = MQTT_QOS_0_AT_MOST_ONCE;
  param.message.topic.topic.utf8 = (uint8_t *)topicName;
  param.message.topic.topic.size =
      strlen(param.message.topic.topic.utf8);

  param.message.payload.data = message;
  param.message.payload.len =
      strlen(param.message.payload.data);

  param.message_id = sys_rand32_get();
  param.dup_flag = 0U;
  param.retain_flag = 0U;

  LOG_INF("Publish topic: %s, message: %s", topicName, message);

  mqtt_publish(&mqttClient, &param);

  return;
}
