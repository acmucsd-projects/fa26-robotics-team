#include "imu.hpp"
#include <Arduino.h>

Imu imu(8, 9);
void MovePos(int id, int position_number);

float Kp = 2.0, Ki = 0.0, Kd = 0.5;
float integral = 0, lastError = 0;

float pid(float target, float measured, float dt) {
  float error = target - measured;
  integral += error * dt;
  float derivative = (error - lastError) / dt;
  lastError = error;
  return Kp * error + Ki * integral + Kd * derivative;
}

const int   SERVO_ID      = 1;
const int   SERVO_CENTER  = 2048;
const float STEPS_PER_DEG = 4096.0 / 360.0;
unsigned long lastTime = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) {
    delay(10);
  }

  if (!imu.begin()) {
    Serial.println("BMI160 init failed");
    while (true) {
      delay(1000);
    }
  }
  Serial.println("Calibrating gyro, keep the sensor still...");
  imu.calibrateGyro();
  Serial.println("BMI160 ready");
}

void loop() {
  ImuReading r;
  if (!imu.read(r)) {
    Serial.println("read failed");
    delay(100);
    return;
  }

  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0;
  lastTime = now;
  if (dt <= 0) return;

  float tilt = atan2(r.ax, r.az) * 180.0 / PI;
  float correction = pid(0.0, tilt, dt);

  int pos = SERVO_CENTER + correction * STEPS_PER_DEG;
  pos = constrain(pos, 1024, 3072);
  MovePos(SERVO_ID, pos);

  Serial.printf("tilt %6.2f  correction %6.2f  pos %d\n", tilt, correction, pos);
  delay(20);
}
