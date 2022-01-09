#ifndef __MQTT_H__
#define __MQTT_H__

int mqttConnect();

void mqttProcess();

void mqttSubscribe(const char* const topicName);

void mqttPublish(const char* const topicName, char* message);

#endif
