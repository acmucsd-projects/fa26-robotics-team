#include "motor.hpp"
#include "offset_source.hpp"
#include <Arduino.h>

// NEW: use motor.cpp exactly as it is. It has its own setup() and loop()
#include <Wire.h>
#include "SCServo.h"
#include <vector>
#define setup motorsBegin
#define loop motorUnusedLoop
#include "motor.cpp"  // gives motorsBegin(), MovePos(), MIN1/MAX1 (pan), MIN2/MAX2 (tilt)
#undef setup
#undef loop

#ifdef USE_IMU
#include "IMU.hpp"
#include "complementary_filter.hpp"
#endif

#ifdef USE_MQTT
#include <PubSubClient.h>
// NEW: these live in mqtt.cpp (unchanged). This just tells main.cpp they exist.
extern PubSubClient client;
void setup_wifi();
void reconnect();
void callback(char *topic, byte *payload, unsigned int length);

// NEW: the latest "x,y" command from MQTT, waiting for loop() to use it. assumption that mqtt gives x,y values for the offset, which is then used to move the gimbal. The values are stored in mqttX and mqttY, and mqttHasCmd indicates if a new command has been received.
float mqttX = 0, mqttY = 0;
bool mqttHasCmd = false;

// NEW: runs when a message arrives. mqtt.cpp's callback() prints it,
// then this reads "x,y" so the gimbal can move.
void onMqttMessage(char *topic, byte *payload, unsigned int length) {
  callback(topic, payload, length);  // mqtt.cpp: print the message
  char msg[32];
  unsigned int n = length < sizeof(msg) - 1 ? length : sizeof(msg) - 1;
  memcpy(msg, payload, n);  // payload has no end marker, so copy it
  msg[n] = '\0';            // and add one
  if (sscanf(msg, "%f,%f", &mqttX, &mqttY) == 2)
    mqttHasCmd = true;
}
#endif

constexpr int SERVO_PAN = 1;
constexpr int SERVO_TILT = 2;
constexpr int STEPS_CENTER = 2048;  // default until motorsBegin() measures the real centres
constexpr float STEPS_PER_DEG = 4096.0f / 360.0f;
constexpr float PAN_GAIN = 20.0f;   // servo steps per offset unit, tune later
constexpr float TILT_GAIN = 20.0f;
constexpr uint32_t LOOP_MS = 10;
constexpr uint32_t OFFSET_TIMEOUT_MS = 500;

float panTarget = STEPS_CENTER;
float tiltTarget = STEPS_CENTER;
uint32_t lastOffsetMs = 0;
float panCenter = STEPS_CENTER;   // NEW: measured centres, set in setup()
float tiltCenter = STEPS_CENTER;

#ifdef USE_IMU
// NEW: the IMU pins (IMU_SDA, IMU_SCL) and address now live in IMU.hpp,
// so they are no longer set here. Change them there if the wiring differs.
// NEW: Imu now takes the accelerometer calibration instead of pins.
// {0,0,0} offset and {1,1,1} scale = no correction yet. After running
// CalibrateIMU, replace this line with the "Imu imu(...)" line it prints.
Imu imu({0, 0, 0}, {1, 1, 1});
ComplementaryFilter filter;
bool imuOk = false;
uint32_t lastImuUs = 0;
uint32_t imuFails = 0;

// NEW: PID for tilt (one copy, keeps its own memory)
struct PID {
  float kp, ki, kd;
  float ierror = 0;      // running total of error (I part)
  float lastError = 0;   // error from the previous update (D part)
  bool first = true;     // no lastError yet: skip D once (avoids a kick)

  PID(float p, float i, float d) : kp(p), ki(i), kd(d) {}

  float computePID(float input, float setPoint, float dt) {
    float error = setPoint - input;
    ierror = constrain(ierror + error * dt, -20.0f, 20.0f);  // stop I growing forever
    float derror = first ? 0.0f : (error - lastError) / dt;
    first = false;
    lastError = error;
    return kp * error + ki * ierror + kd * derror;
  }
};

PID tiltPid{1.0f, 4.0f, 0.0f};    // kp, ki, kd: starting values, tune on the real head
constexpr float TILT_SIGN = 1.0f; // flip to -1 if tilt corrects the wrong way
float tiltCorrDeg = 0;            // latest PID correction, for the printout
#endif

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

  motorsBegin();  // motor.cpp: finds MIN1/MAX1 and MIN2/MAX2, centres both servos

  // NEW: if a servo didn't answer, its limits come out tiny or backwards: use full range
  if (MAX1 - MIN1 < 500) { MIN1 = 0; MAX1 = 4095; }
  if (MAX2 - MIN2 < 500) { MIN2 = 0; MAX2 = 4095; }
  panCenter = (MIN1 + MAX1) / 2;
  tiltCenter = (MIN2 + MAX2) / 2;
  panTarget = panCenter;    // start where motor.cpp left the servos (no jump)
  tiltTarget = tiltCenter;

#ifdef USE_MQTT
  setup_wifi();                               // NEW: mqtt.cpp (waits until Wi-Fi connects)
  client.setServer("broker.emqx.io", 1883);   // NEW: which broker
  client.setCallback(onMqttMessage);          // NEW: who gets the messages
#endif

#ifdef USE_IMU
  lastImuUs = micros();  // motorsBegin() takes seconds; don't count it as dt
#endif
}

void loop() {
#ifdef USE_MQTT
  if (!client.connected())
    reconnect();   // NEW: mqtt.cpp (waits until the broker answers)
  client.loop();   // NEW: receive messages (calls onMqttMessage)
#endif

  static uint32_t lastTick = 0;
  if (millis() - lastTick < LOOP_MS)
    return;
  lastTick = millis();

  // Open loop: offset -> servo targets.
  Offset o;
  bool gotOffset = pollOffset(o);
#ifdef USE_MQTT
  if (!gotOffset && mqttHasCmd) {  // NEW: no local offset? use the MQTT command
    o.x = mqttX;
    o.y = mqttY;
    mqttHasCmd = false;
    gotOffset = true;
  }
#endif
  if (gotOffset) {  // CHANGED: was if (pollOffset(o))
    lastOffsetMs = millis();
    panTarget = constrain(panTarget + PAN_GAIN * o.x, MIN1, MAX1);     // CHANGED: measured limits
    tiltTarget = constrain(tiltTarget + TILT_GAIN * o.y, MIN2, MAX2);
    MovePos(SERVO_PAN, (int)panTarget);
#ifdef USE_IMU
    if (!imuOk)  // CHANGED: with a working IMU, the PID below moves tilt
#endif
      MovePos(SERVO_TILT, (int)tiltTarget);
  }
  if (millis() - lastOffsetMs > OFFSET_TIMEOUT_MS) {
    // No offsets lately: hold position.
  }

#ifdef USE_IMU
  // Read the IMU, and (NEW) let the PID hold the tilt on target.
  static ImuReading r;
  if (imuOk) {
    if (imu.read(r)) {
      uint32_t nowUs = micros();
      float dt = (nowUs - lastImuUs) / 1e6f;
      lastImuUs = nowUs;
      filter.update(r.accel.x, r.accel.y, r.accel.z, r.gyro.y, r.gyro.z, dt);

      // NEW: target vs measured tilt -> correction -> servo
      if (dt > 0 && dt < 0.1f) {  // skip glitches and long gaps
        float targetDeg = (tiltTarget - tiltCenter) / STEPS_PER_DEG;
        tiltCorrDeg = TILT_SIGN * tiltPid.computePID(filter.tilt, targetDeg, dt);
        int pos = constrain((int)(tiltTarget + tiltCorrDeg * STEPS_PER_DEG), MIN2, MAX2);
        MovePos(SERVO_TILT, pos);
      }
    } else {
      imuFails++;
    }
  }

  static uint32_t lastPrint = 0;
  if (millis() - lastPrint >= 200) {
    lastPrint = millis();
    Serial.printf("cmd tilt %6.1f pan %6.1f | imu tilt %6.1f pan %6.1f | corr %6.1f | "
                  "a %6.2f %6.2f %6.2f | g %7.2f %7.2f %7.2f | fails %lu\n",
                  (tiltTarget - tiltCenter) / STEPS_PER_DEG,
                  (panTarget - panCenter) / STEPS_PER_DEG, filter.tilt,
                  filter.pan, tiltCorrDeg, r.accel.x, r.accel.y, r.accel.z, r.gyro.x,
                  r.gyro.y, r.gyro.z, (unsigned long)imuFails);
  }
#endif
}