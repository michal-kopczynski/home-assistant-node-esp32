#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#include <stdbool.h>
#include <stddef.h>

struct MessageSensorsData
{
  unsigned int pms1_0;
  unsigned int pms2_5;
  unsigned int pms10;
  int temperature;
  unsigned int humidity;
};

struct MessageNodeData
{
  const char *location;
  const char *timestamp;
  const struct MessageSensorsData sensors;
};

bool messageEncodeNodeData(
  const struct MessageNodeData* response,
  char * const buffer,
  const size_t bufferSize);

#endif
