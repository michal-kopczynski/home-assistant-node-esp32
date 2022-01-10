#ifndef __SENSORS_H__
#define __SENSORS_H__

#include <stdbool.h>

struct SensorsData
{
  unsigned int pms1_0;
  unsigned int pms2_5;
  unsigned int pms10;
  int temperature;
  unsigned int humidity;
};

void initSensors();

bool getSensorsData(struct SensorsData* const data);

#endif
