#include "ComplimentaryFilter.hpp"
#include "IMU.hpp"
#include <Arduino.h>
#include "mqtt.cpp"

Imu imu({31.57, -498.17, 91.18}, {1.0106, 1.0074, 0.9943});
ComplementaryFilter filter;

// NEW: servo limits, in degrees from centre (match motor.cpp's MovePos)
constexpr float PAN_LIMIT_DEG = 90.0f;   // pan (yaw):   -90 to +90
constexpr float TILT_LIMIT_DEG = 45.0f;  // tilt (pitch): -45 to +45

// NEW: where we want to look, in degrees (0 = straight ahead and level)
float panTargetDeg = 0;
float tiltTargetDeg = 0;

// NEW: PID for tilt (one copy, keeps its own memory)
struct PID {
  float kp, ki, kd;
  float ierror = 0;      // running total of error (I part)
  float lastError = 0;   // error from the previous update (D part)
  bool first = true;     // no lastError yet: skip D once (avoids a kick)

  PID(float p, float i, float d) : kp(p), ki(i), kd(d) {}

  float computePID(float input, float setPoint, float dt) {
    float error = setPoint - input;
    ierror = constrain(ierror + error * dt, -10.0f, 10.0f);  // stop I growing forever
    float derror = first ? 0.0f : (error - lastError) / dt;
    first = false;
    lastError = error;
    return kp * error + ki * ierror + kd * derror;
  }
};

PID tiltPid{0.5f, 4.0f, 0.0f};     // NEW: kp, ki, kd: starting values, tune on the real head
constexpr float TILT_SIGN = 1.0f;  // NEW: flip to -1 if tilt corrects the wrong way
float tiltCorrDeg = 0;             // NEW: latest PID correction
float panCmdDeg = 0;               // NEW: the command for the servos, inside the limits
float tiltCmdDeg = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) {
  }

  motorsBegin();

  // Setup: calls the mqtt server and designated port in MQTT.cpp
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  if (!imu.begin()) {
    Serial.println("BMI160 init failed");
    while (true) {
    }
  }
  Serial.println("Calibrating gyro, keep the sensor still...");
  imu.calibrateGyro();
  Serial.println("BMI160 ready");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  while (espClient.available()) {
    client.loop();
  }

  // CHANGED: a new MQTT target now sets where we WANT to look.
  // The PID below does the moving, so tilt holds its angle even if the base moves.
  if (newTarget) {
    newTarget = false;
    panTargetDeg = constrain(targetYaw, -PAN_LIMIT_DEG, PAN_LIMIT_DEG);        // NEW
    tiltTargetDeg = constrain(targetPitch, -TILT_LIMIT_DEG, TILT_LIMIT_DEG);   // NEW
  }

  ImuReading r;
  if (!imu.read(r)) {
    Serial.println("read failed");
    // Wait for the next IMU sample (100 Hz) so a dead IMU doesn't flood serial.
    delay(10);
    return;
  }

  // dt = seconds since the last loop. The filter needs it to turn deg/s into deg.
  static uint32_t last = micros();
  uint32_t now = micros();
  filter.update(r, (now - last) / 1e6f);
  last = now;

  // NEW: every 10 ms, the PID compares wanted tilt with the filter's tilt,
  // adds the fix, keeps it inside the limits and moves both servos.
  static uint32_t lastPidUs = micros();
  float pidDt = (now - lastPidUs) / 1e6f;   // seconds since the last PID update
  if (pidDt >= 0.01f) {                     // 10 ms = 100 times a second
    lastPidUs = now;
    if (pidDt < 0.1f) {                     // skip long gaps (e.g. after a Wi-Fi reconnect)
      tiltCorrDeg = TILT_SIGN * tiltPid.computePID(filter.tilt, tiltTargetDeg, pidDt);
    }
    panCmdDeg = constrain(panTargetDeg, -PAN_LIMIT_DEG, PAN_LIMIT_DEG);
    tiltCmdDeg = constrain(tiltTargetDeg + tiltCorrDeg, -TILT_LIMIT_DEG, TILT_LIMIT_DEG);
    MovePos(panCmdDeg, tiltCmdDeg);
  }

  // Print at most every 200 ms so serial doesn't slow the loop down.
//   static unsigned long lastPrint = 0;
//   if (millis() - lastPrint >= 200) {
//     lastPrint = millis();
//     Serial.printf("accel (m/s^2): %7.2f %7.2f %7.2f  |a| %6.2f   gyro (deg/s): "
//                   "%8.2f %8.2f %8.2f   tilt %7.2f  pan %7.2f\n",
//                   r.accel.x, r.accel.y, r.accel.z,
//                   sqrtf(r.accel.x * r.accel.x + r.accel.y * r.accel.y +
//                         r.accel.z * r.accel.z),
//                   r.gyro.x, r.gyro.y, r.gyro.z, filter.tilt, filter.pan); // gyro x y z
//   }
}