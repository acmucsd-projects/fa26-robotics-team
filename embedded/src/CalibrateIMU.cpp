// run: pio run -e calibrate -t upload -t monitor
// Point each axis up, hold still, then down. Gravity is exactly 1g, so the up/down
// readings tell us each axis's offset (middle) and scale (stretch).
// At the end it prints an initialization line to paste into Imu's constructor.

#include "Array.hpp"
#include "IMU.hpp"
#include "Vector.hpp"
#include <Arduino.h>

Imu imu;

// Repeats the prompt until something is typed in the serial monitor
void waitForEnter(const char *prompt) {
  while (Serial.available()) {
    Serial.read();
  }

  while (!Serial.available()) {
    Serial.println(prompt);
    delay(5000); // During calibration, it stops "Point _ up, hold still, then hit enter"
                 // printing rapidly thousands of times
  }
}

// Waits for the user, hold still, then averages the accel while the board sits still
bool measure(const char *prompt, Vec3<> &gyro, Vec3<> &accel) {
  waitForEnter(prompt);
  Serial.println("Measuring, hold still...");
  if (!imu.avgRaw(gyro, accel)) {
    return false;
  };
  Serial.println("Done");
  return true;
}

struct AxisStep {
  const char *prompt;
  Vec3<> *out;
};

void setup() {
  Serial.begin(115200);
  if (!imu.begin()) {
    while (true) {
      Serial.println("BMI160 init failed");
      delay(1000); // prevents "BMI160 init failed" from printing rapidly
    }
  }

  Vec3<> gyroBias;
  Vec3<> gyro;
  const Array<char, 3> axes{'X', 'Y', 'Z'};
  const Array<char, 2> signs{'+', '-'};
  Array<Array<Vec3<>, 3>, 2> samples{}; // [0 = +, 1 = -][0 = X, 1 = Y, 2 = Z]
  Array<char, 43> prompt{};

  for (size_t axis{}; axis < axes.size(); ++axis) {
    for (size_t sign{}; sign < signs.size(); ++sign) {
      snprintf(prompt.data(), prompt.size(), "Point %c%c up, hold still, then hit enter",
               signs[sign], axes[axis]);
      if (!measure(prompt.data(), gyro, samples[sign][axis])) {
        return;
      }
      gyroBias += gyro;
    }
  }

  gyroBias /= 6.0F;

  // Each axis reads +1g up and -1g down. Bias = middle of the two,
  // scale = what turns half the gap into exactly 1g
  Vec3<> positives{samples[0][0].x, samples[0][1].y, samples[0][2].z};
  Vec3<> negatives{samples[1][0].x, samples[1][1].y, samples[1][2].z};
  Vec3<> accelBias{(positives + negatives) / 2.0F};
  Vec3<> accelScale{Vec3<>{ACCEL_LSB_PER_G * 2.0F} / (positives - negatives)};

  Serial.println("Calibration done, paste this into Imu's constructor");
  Serial.printf("{%.4f, %.4f, %.4f}, {%.4f, %.4f, %.4f}, {%.4f, %.4f, %.4f}\n", gyroBias.x,
                gyroBias.y, gyroBias.z, accelBias.x, accelBias.y, accelBias.z, accelScale.x,
                accelScale.y, accelScale.z);
}

void loop() {}
