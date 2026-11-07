
#include <Arduino.h>
#include <Wire.h>
#include "SCServo.h"

SMS_STS st;

#define SERVO_RX 15
#define SERVO_TX 16

void setup() {
Serial.begin(115200);

const int SERVO_ID = 2;
const int SERVO_CENTER = 390;

Serial1.begin(1000000, SERIAL_8N1, SERVO_RX, SERVO_TX);
st.pSerial = &Serial1;
st.WritePosEx(SERVO_ID, SERVO_CENTER, 2000, 50);
delay(500);
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
	const int SERVO_ID = 2;
	const int SERVO_CENTER = 390;
	Serial.println(st.ReadPos(2));
	
	st.WritePosEx(SERVO_ID, 2411, 2000, 50);
	delay(2000);
	Serial.println(st.ReadPos(2));

	st.WritePosEx(SERVO_ID, 380, 2000, 50);
	delay(2000);
}