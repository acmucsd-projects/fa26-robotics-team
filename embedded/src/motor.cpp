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
int MIN2 = 500;
int MAX2 = 500;
const int SERVO1 = 1;
const int SERVO2 = 2;
const int SERVO_INVCENTER = 2438;
const int MAX_CURR = 8;

void MovePos(int id, int position) {
	st.WritePosEx(id, position, 2000, 50);
}

void setup() {
	// baud rate 115200
	Serial.begin(115200);

	Serial1.begin(1000000, SERIAL_8N1, SERVO_RX, SERVO_TX);
	st.pSerial = &Serial1;
	MovePos(SERVO1, 2422);
	MovePos(SERVO2, 2438);
	delay(1500);
	Serial.println("Scanning bus");

	for(int id = 1; id <= 20; id++) {
		if (st.Ping(id)==id) {
			Serial.print("Found servo ID:");
			Serial.println(id);
		}
	}

	int currPos1 = SERVO_INVCENTER;
	int currPos2 = SERVO_INVCENTER;
	bool moving1 = true;
	bool moving2 = true;

	delay(500);

	// ---- find MIN1 / MIN2 (both servos stepping together) ----
	while (moving1 || moving2) {
		if (moving1) {
			if (st.ReadCurrent(SERVO1) < MAX_CURR && st.ReadCurrent(SERVO1) != -1) {
				MovePos(SERVO1, currPos1-10);
				currPos1 -= 10;
				Serial.print("servo1 ");
				Serial.print(currPos1);
				Serial.print(" ");
				Serial.println(st.ReadCurrent(SERVO1));
			} else {
				MIN1 = currPos1 + 10;
				MovePos(SERVO1, SERVO_INVCENTER);
				currPos1 = SERVO_INVCENTER;
				moving1 = false;
			}
		}

		if (moving2) {
			if (st.ReadCurrent(SERVO2) < MAX_CURR && st.ReadCurrent(SERVO2) != -1) {
				MovePos(SERVO2, currPos2-10);
				currPos2 -= 10;
				Serial.print("servo2 ");
				Serial.print(currPos2);
				Serial.print(" ");
				Serial.println(st.ReadCurrent(SERVO2));
			} else {
				MIN2 = currPos2 + 10;
				currPos2 = SERVO_INVCENTER;
				moving2 = false;
			}
		}

		delay(20);
	}

	MovePos(SERVO2, SERVO_INVCENTER);
	delay(1000);

	moving1 = true;
	moving2 = true;

	// ---- find MAX1 / MAX2 (both servos stepping together) ----
	while (moving1 || moving2) {
		if (moving1) {
			if (st.ReadCurrent(SERVO1) < MAX_CURR && st.ReadCurrent(SERVO1) != -1) {
				MovePos(SERVO1, currPos1+10);
				currPos1 += 10;
				Serial.print("servo1 ");
				Serial.print(currPos1);
				Serial.print(" ");
				Serial.println(st.ReadCurrent(SERVO1));
			} else {
				MAX1 = currPos1 - 10;
				MovePos(SERVO1, SERVO_INVCENTER);
				currPos1 = SERVO_INVCENTER;
				moving1 = false;
			}
		}

		if (moving2) {
			if (st.ReadCurrent(SERVO2) < MAX_CURR && st.ReadCurrent(SERVO2) != -1) {
				MovePos(SERVO2, currPos2+10);
				currPos2 += 10;
				Serial.print("servo2 LOOP 2 ");
				Serial.println(st.ReadCurrent(SERVO2));
			} else {
				MAX2 = currPos2 - 10;
				currPos2 = SERVO_INVCENTER;
				moving2 = false;
			}
		}

		delay(20);
	}

	MovePos(SERVO1, (MAX1+MIN1)/2);
	MovePos(SERVO2, (MAX2+MIN2)/2);

	Serial.println(MIN1);
	Serial.println(MAX1);
	Serial.println(MIN2);
	Serial.println(MAX2);

}

void loop() {
	
}