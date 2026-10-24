
#include <Arduino.h>
#include <Wire.h>
#include "SCServo.h"

SMS_STS st;

#define SERVO_RX 15
#define SERVO_TX 16

void setup() {
Serial.begin(115200);

Serial1.begin(1000000, SERIAL_8N1, SERVO_RX, SERVO_TX);
st.pSerial = &Serial1;
delay(1500);
Serial.println("Scanning bus");

for(int id = 1; id <= 20; id++) {
  if (st.Ping(id)==id) {
    Serial.print("Found servo ID:");
    Serial.println(id);
  }
}
}

void loop() {
  int id = 1;
  while (id <= 2) {                  // servos 1 and 2 -> first position
    st.WritePosEx(id, 2048, 2000, 50);
    id++;
  }
  delay(1000);                       // give them time to get there

  id = 1;
  while (id <= 2) {                  // servos 1 and 2 -> second position
    st.WritePosEx(id, 3000, 2000, 50);
    id++;
  }
  delay(1000);
}