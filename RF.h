#pragma once

#include <stdint.h>

struct TelemetrySample {
  uint32_t timeMs;
  float temperatureC;
  int32_t pressurePa;
  float heightM;
  float accelerationG[3];
  float gyroDps[3];
};

bool beginRF();
void sendRF(const TelemetrySample &sample);
