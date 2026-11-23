// #include "motor.hpp"
// // #include "offset_source.hpp"
// #include <Arduino.h>

// #ifdef USE_IMU
// #include "IMU.hpp"
// #include "complementary_filter.hpp"
// #endif

// constexpr int SERVO_PAN = 1;
// constexpr int SERVO_TILT = 2;
// constexpr int STEPS_CENTER = 2048;  // TODO: use measured centers
// constexpr float STEPS_PER_DEG = 4096.0f / 360.0f;
// constexpr float PAN_GAIN = 20.0f;   // servo steps per offset unit, tune later
// constexpr float TILT_GAIN = 20.0f;
// constexpr uint32_t LOOP_MS = 10;
// constexpr uint32_t OFFSET_TIMEOUT_MS = 500;

// float panTarget = STEPS_CENTER;
// float tiltTarget = STEPS_CENTER;
// uint32_t lastOffsetMs = 0;

// #ifdef USE_IMU
// // TODO: confirm the pins. CalibrateIMU.cpp uses 6, 7; the PR's main.cpp used 8, 9.
// constexpr uint8_t IMU_SDA = 6;
// constexpr uint8_t IMU_SCL = 7;
// Imu imu(IMU_SDA, IMU_SCL);
// ComplementaryFilter filter;
// bool imuOk = false;
// uint32_t lastImuUs = 0;
// uint32_t imuFails = 0;

// // NEW ---------- PID for tilt (one copy, keeps its own memory) ----------
// struct PID {
//   float kp, ki, kd;
//   float ierror = 0;      // running total of error (I part)
//   float lastError = 0;   // error from the previous update (D part)
//   bool first = true;     // no lastError yet: skip D once (avoids a kick)

//   PID(float p, float i, float d) : kp(p), ki(i), kd(d) {}

//   float computePID(float input, float setPoint, float dt) {
//     float error = setPoint - input;
//     ierror = constrain(ierror + error * dt, -20.0f, 20.0f);  // stop I growing forever
//     float derror = first ? 0.0f : (error - lastError) / dt;
//     first = false;
//     lastError = error;
//     return kp * error + ki * ierror + kd * derror;
//   }
// };

// PID tiltPid{1.0f, 4.0f, 0.0f};    // kp, ki, kd: starting values, tune on the real head
// constexpr float TILT_SIGN = 1.0f; // flip to -1 if tilt corrects the wrong way
// float tiltCorrDeg = 0;            // latest PID correction, for the printout
// #endif

// // NEW: is tilt being driven by the PID right now?
// bool tiltClosedLoop() {
// #ifdef USE_IMU
//   return imuOk;
// #else
//   return false;
// #endif
// }

// void setup() {
//   Serial.begin(115200);

// #ifdef USE_IMU
//   // IMU first: the head must be still while the gyro zeroes, and
//   // motorsBegin() moves the servos.
//   imuOk = imu.begin();
//   if (imuOk) {
//     Serial.println("Zeroing gyro, keep the head still...");
//     imu.calibrateGyro();
//   } else {
//     Serial.println("BMI160 init failed, running without IMU");
//   }
// #endif

// //   motorsBegin();

// #ifdef USE_IMU
//   lastImuUs = micros();  // motorsBegin() takes seconds; don't count it as dt
// #endif
// }

// void loop() {
//   static uint32_t lastTick = 0;
//   if (millis() - lastTick < LOOP_MS)
//     return;
//   lastTick = millis();

//   // Offset -> servo targets.
//   Offset o;
//   if (pollOffset(o)) {
//     lastOffsetMs = millis();
//     panTarget = constrain(panTarget + PAN_GAIN * o.x, 0, 4095);
//     tiltTarget = constrain(tiltTarget + TILT_GAIN * o.y, 0, 4095);
//     MovePos(SERVO_PAN, (int)panTarget);
//     if (!tiltClosedLoop())                // CHANGED: with the IMU, the PID moves tilt
//       MovePos(SERVO_TILT, (int)tiltTarget);
//   }
//   if (millis() - lastOffsetMs > OFFSET_TIMEOUT_MS) {
//     // No offsets lately: hold position.
//   }

// #ifdef USE_IMU
//   // Read the IMU, update the filter, and (NEW) correct tilt with the PID.
//   static ImuReading r;
//   if (imuOk) {
//     if (imu.read(r)) {
//       uint32_t nowUs = micros();
//       float dt = (nowUs - lastImuUs) / 1e6f;
//       lastImuUs = nowUs;
//       filter.update(r.accel.x, r.accel.y, r.accel.z, r.gyro.y, r.gyro.z, dt);

//       // NEW: target vs measured tilt -> correction -> servo
//       if (dt > 0 && dt < 0.1f) {  // skip glitches and long gaps
//         float targetDeg = (tiltTarget - STEPS_CENTER) / STEPS_PER_DEG;
//         tiltCorrDeg = TILT_SIGN * tiltPid.computePID(filter.tilt, targetDeg, dt);
//         int pos = constrain((int)(tiltTarget + tiltCorrDeg * STEPS_PER_DEG), 0, 4095);
//         MovePos(SERVO_TILT, pos);
//       }
//     } else {
//       imuFails++;
//     }
//   }

//   static uint32_t lastPrint = 0;
//   if (millis() - lastPrint >= 200) {
//     lastPrint = millis();
//     Serial.printf("cmd tilt %6.1f pan %6.1f | imu tilt %6.1f pan %6.1f | corr %6.1f | "
//                   "a %6.2f %6.2f %6.2f | g %7.2f %7.2f %7.2f | fails %lu\n",
//                   (tiltTarget - STEPS_CENTER) / STEPS_PER_DEG,
//                   (panTarget - STEPS_CENTER) / STEPS_PER_DEG, filter.tilt,
//                   filter.pan, tiltCorrDeg, r.accel.x, r.accel.y, r.accel.z, r.gyro.x,
//                   r.gyro.y, r.gyro.z, (unsigned long)imuFails);
//   }
// #endif
// }