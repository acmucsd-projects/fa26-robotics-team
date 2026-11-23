<<<<<<< HEAD
#include "motor.hpp"
#include "offset_source.hpp"
#include <Arduino.h>

#ifdef USE_IMU
#include "IMU.hpp"
#include "complementary_filter.hpp"
#endif

constexpr int SERVO_PAN = 1;
constexpr int SERVO_TILT = 2;
constexpr int STEPS_CENTER = 2048;  // TODO: use measured centers
constexpr float STEPS_PER_DEG = 4096.0f / 360.0f;
constexpr float PAN_GAIN = 20.0f;   // servo steps per offset unit, tune later
constexpr float TILT_GAIN = 20.0f;
constexpr uint32_t LOOP_MS = 10;
constexpr uint32_t OFFSET_TIMEOUT_MS = 500;

float panTarget = STEPS_CENTER;
float tiltTarget = STEPS_CENTER;
uint32_t lastOffsetMs = 0;

#ifdef USE_IMU
constexpr uint8_t IMU_SDA = 6;
constexpr uint8_t IMU_SCL = 7;
Imu imu(IMU_SDA, IMU_SCL);
ComplementaryFilter filter;
bool imuOk = false;
uint32_t lastImuUs = 0;
uint32_t imuFails = 0;
#endif
=======
#include "IMU.hpp"
#include "complimentary_filter.hpp"
#include <Arduino.h>

/* sda 6, scl 7, replace with the line the calibrate firmware prints */
Imu imu(6, 7);
ComplementaryFilter filter;
>>>>>>> aubrey

void setup() {
  Serial.begin(115200);

#ifdef USE_IMU
  // IMU first: the head must be still while the gyro zeroes, and
  // motorsBegin() moves the servos.
  imuOk = imu.begin();
  if (imuOk) {
    Serial.println("Zeroing gyro, keep the head still...");
    imu.calibrateGyro();
  } else {
    Serial.println("BMI160 init failed, running without IMU");
  }
#endif

  motorsBegin();

#ifdef USE_IMU
  lastImuUs = micros();  // motorsBegin() takes seconds; don't count it as dt
#endif
}

void loop() {
  static uint32_t lastTick = 0;
  if (millis() - lastTick < LOOP_MS)
    return;
  lastTick = millis();

  // Open loop: offset -> servo targets.
  Offset o;
  if (pollOffset(o)) {
    lastOffsetMs = millis();
    panTarget = constrain(panTarget + PAN_GAIN * o.x, 0, 4095);
    tiltTarget = constrain(tiltTarget + TILT_GAIN * o.y, 0, 4095);
    MovePos(SERVO_PAN, (int)panTarget);
    MovePos(SERVO_TILT, (int)tiltTarget);
  }
  if (millis() - lastOffsetMs > OFFSET_TIMEOUT_MS) {
    // No offsets lately: hold position.
  }

<<<<<<< HEAD
#ifdef USE_IMU
  // Observe only: read the IMU and print it next to what we commanded.
  // Nothing here moves the servos yet.
  static ImuReading r;
  if (imuOk) {
    if (imu.read(r)) {
      uint32_t nowUs = micros();
      float dt = (nowUs - lastImuUs) / 1e6f;
      lastImuUs = nowUs;
      filter.update(r.accel.x, r.accel.y, r.accel.z, r.gyro.y, r.gyro.z, dt);
    } else {
      imuFails++;
    }
  }

  static uint32_t lastPrint = 0;
  if (millis() - lastPrint >= 200) {
    lastPrint = millis();
    Serial.printf("cmd tilt %6.1f pan %6.1f | imu tilt %6.1f pan %6.1f | "
                  "a %6.2f %6.2f %6.2f | g %7.2f %7.2f %7.2f | fails %lu\n",
                  (tiltTarget - STEPS_CENTER) / STEPS_PER_DEG,
                  (panTarget - STEPS_CENTER) / STEPS_PER_DEG, filter.tilt,
                  filter.pan, r.accel.x, r.accel.y, r.accel.z, r.gyro.x,
                  r.gyro.y, r.gyro.z, (unsigned long)imuFails);
  }
#endif
=======
  /* dt = seconds since the last loop, the filter needs it to turn deg/s into
   * deg */
  static uint32_t last = micros();
  uint32_t now = micros();
  /* gyro y = tilt, gyro z = pan */
  filter.update(r.accel.x, r.accel.y, r.accel.z, r.gyro.y, r.gyro.z,
                (now - last) / 1e6f);
  last = now;

  /* |a| should be about 9.81 when the board is still */
  Serial.printf("accel (m/s^2): %7.2f %7.2f %7.2f  |a| %6.2f   gyro (deg/s): "
                "%8.2f %8.2f %8.2f   tilt %7.2f  pan %7.2f\n",
                r.accel.x, r.accel.y, r.accel.z,
                sqrtf(r.accel.x * r.accel.x + r.accel.y * r.accel.y +
                      r.accel.z * r.accel.z),
                r.gyro.x, r.gyro.y, r.gyro.z, filter.tilt, filter.pan);
  /* ~100hz, matches the imu rate */
  delay(10);
>>>>>>> aubrey
}
