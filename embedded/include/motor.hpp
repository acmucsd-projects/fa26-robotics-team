#include <Arduino.h>
#include <Wire.h>
#include "SCServo.h"

// motor.hpp
#pragma once
#include <array>



void MovePos(float yaw, float pitch);
void motorsBegin();