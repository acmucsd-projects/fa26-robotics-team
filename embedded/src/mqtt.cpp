#include "mqtt.hpp"
#include <Arduino.h>
#include <math.h>

// Pretends to be the MQTT client: ~30 offsets/s, slow sweep, 2 s dropout
// every 10 s so the lost-offset path gets exercised.
bool pollOffset(Offset &out) {
  static uint32_t last = 0;
  uint32_t now = millis();
  if (now - last < 33)
    return false;
  last = now;

  float t = now / 1000.0f;
  if (fmodf(t, 10.0f) > 8.0f)
    return false;

  out.x = 0.5f * sinf(t * 0.8f);
  out.y = 0.3f * sinf(t * 0.5f);
  return true;
}