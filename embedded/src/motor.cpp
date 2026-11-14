
#include <Arduino.h>
#include <Wire.h>
#include "SCServo.h"
#include <vector>

SMS_STS st;

#define SERVO_RX 15
#define SERVO_TX 16

const int SPEED = 4000;
int MIN1 = 500;
int MAX1 = 500;
const int SERVO1 = 1;
const int SERVO2 = 2;
const int SERVO_INVCENTER = 2438;

void MovePos(int id, int position) {
	st.WritePosEx(id, position, 2000, 50);
}

void setup() {
	// baud rate 115200
	Serial.begin(115200);

	Serial1.begin(1000000, SERIAL_8N1, SERVO_RX, SERVO_TX);
	st.pSerial = &Serial1;
	// st.WritePosEx(SERVO1, SERVO_CENTER, 2000, 50);
	MovePos(SERVO2, 2438);
	delay(1500);
	Serial.println("Scanning bus");

	for(int id = 1; id <= 20; id++) {
		if (st.Ping(id)==id) {
			Serial.print("Found servo ID:");
			Serial.println(id);
		}
	}

	int currPos = st.ReadPos(SERVO2);

	delay(500);

	while (st.ReadCurrent(SERVO2) < 7) {
		MovePos(SERVO2, currPos-10);
		currPos -= 10;
		Serial.print("servo2 ");
		Serial.print(currPos);
		Serial.print(" ");
		Serial.println(st.ReadCurrent(SERVO2));
		delay(20);
	}

	MIN1 = currPos + 10;
	currPos = SERVO_INVCENTER;

	MovePos(SERVO2, 2438);
	delay(1000);

	while (st.ReadCurrent(SERVO2) < 7) {
		MovePos(SERVO2, currPos+10);
		currPos += 10;
		Serial.print("servo2 LOOP 2 ");
		Serial.println(st.ReadCurrent(SERVO2));
		delay(20);
	}
	MAX1 = currPos - 10;

	MovePos(SERVO2, (MAX1+MIN1)/2);

	Serial.println(MIN1);
	Serial.println(MAX1);

}

void loop() {
	
}

