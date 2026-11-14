#include <Arduino.h>
#include <Wire.h>
#include "SCServo.h"

// motor.hpp
#pragma once
#include <array>

struct ServoLimits {
  int min;
  int max;
  int center;
};

void MovePos(int id, int position);
ServoLimits calibrateServo(int id, int startPos);
