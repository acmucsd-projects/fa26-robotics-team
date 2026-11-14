#include <Arduino.h>
#include "IMU.hpp"
#include <motor.hpp>

Imu imu(8, 9);

// ---------- PID (one copy per motor) ----------
// each motor keeps its own memory (ierror, lastError).
struct PID {
  float kp, ki, kd;
  float ierror = 0;      // running total of error (I part)
  float lastError = 0;   // error from the previous loop (D part)

  float computePID(float input, float setPoint, float elapsedTime) {
    float error  = setPoint - input;
    ierror += error * elapsedTime;
    ierror = constrain(ierror, -50, 50);   // stop the I part growing forever
    float derror = (error - lastError) / elapsedTime;
    lastError = error;
    return kp * error + ki * ierror + kd * derror;
  }
};

PID pid1{2.0, 0.0, 0.5};   // motor 1: pitch (forward / back)
PID pid2{2.0, 0.0, 0.5};   // motor 2: roll  (left / right)

// ---------- Servo settings ----------
const int   SERVO1_ID     = 1;
const int   SERVO2_ID     = 2;
const int   SERVO_CENTER  = 2438;
const float STEPS_PER_DEG = 4096.0 / 360.0;
unsigned long previousTime = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) {
    delay(10);
  }

  // IMU
  if (!imu.begin()) {
    Serial.println("BMI160 init failed");
    while (true) {
      delay(1000);
    }
  }
  Serial.println("Calibrating gyro, keep the sensor still...");
  imu.calibrateGyro();
  Serial.println("BMI160 ready");


  delay(500);

  previousTime = millis();
}

void loop() {
  ImuReading r;
  if (!imu.read(r)) {
    Serial.println("read failed");
    delay(100);
    return;
  }

  // time since last loop, in seconds
  unsigned long currentTime = millis();
  float elapsedTime = (currentTime - previousTime) / 1000.0;
  previousTime = currentTime;
  if (elapsedTime <= 0) return;

  // two tilt angles from the accelerometer
  float pitch = atan2(-r.ax, sqrt(r.ay * r.ay + r.az * r.az)) * 180.0 / PI;
  float roll  = atan2(r.ay, r.az) * 180.0 / PI;

  // each motor gets its own PID, target = 0 degrees (level)
  float output1 = pid1.computePID(pitch, 0.0, elapsedTime);
  float output2 = pid2.computePID(roll,  0.0, elapsedTime);

  ServoLimits lim1 = calibrateServo(SERVO1_ID, 2438);
  ServoLimits lim2 = calibrateServo(SERVO2_ID, 2438);

  Serial.printf("pitch %6.2f roll %6.2f | pos1 %d pos2 %d\n",
                pitch, roll, lim1.center, lim2.center);
  delay(20);   // about 50 loops per second
}