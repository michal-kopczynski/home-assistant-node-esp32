#ifndef __SENSORS_H__
#define __SENSORS_H__

#include <stdbool.h>

struct sensorsData
{
  unsigned int pms1_0;
  unsigned int pms2_5;
  unsigned int pms10;
  unsigned int temperature;
  unsigned int humidity;
};

void initSensors();

bool getSensorsData(struct sensorsData* const data);

#endif
