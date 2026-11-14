
#include <Arduino.h>
#include <Wire.h>
#include "SCServo.h"
#include <vector>
#include <motor.hpp>

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

ServoLimits calibrateServo(int id, int startPos) {
	int currPos = startPos;
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

	MovePos(SERVO2, startPos);
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
	
	return(ServoLimits{MAX1, MIN1, (MAX1+MIN1)/2});

}


