#include "config.h"
#include <AFMotor.h>
#include <SerialTransfer.h>

int LIMIT_F = 0;
int LIMIT_B = 0;
int LIMIT_U = 0;
int LIMIT_D = 0;
int LIMIT_L = 0;

AF_DCMotor MT_UD(1, MOTOR12_64KHZ);
AF_DCMotor CLAW(2, MOTOR12_64KHZ);
AF_DCMotor MT_FB(3, MOTOR34_64KHZ);
AF_DCMotor MT_LR(4, MOTOR34_64KHZ);

bool clawParked = false;
bool gantryParked = false;

SerialTransfer com;

void setup() {
	pinMode(SW_LIMIT_F_PIN, INPUT_PULLUP); // Gantry Forward Limit
	pinMode(SW_LIMIT_B_PIN, INPUT_PULLUP); // Gantry Backward Limit
	pinMode(SW_LIMIT_U_PIN, INPUT_PULLUP); // Claw Up Limit
	pinMode(SW_LIMIT_D_PIN, INPUT_PULLUP); // Claw Down Limit
	pinMode(SW_LIMIT_L_PIN, INPUT_PULLUP); // Gantry Left Limit
	readLimitSwitches();				   // Get the initial limit switch state.

	startCom();
	setDefaultSpeed();
	clack();

	parkAll();

	pinMode(LED_BUILTIN, OUTPUT);
}

void startCom() {
	Serial.begin(115200);
	com.begin(Serial);
}

void loop() {
	readLimitSwitches();
}

void readLimitSwitches() {
	LIMIT_F = digitalRead(SW_LIMIT_F_PIN);
	LIMIT_B = digitalRead(SW_LIMIT_B_PIN);
	LIMIT_U = digitalRead(SW_LIMIT_U_PIN);
	LIMIT_D = digitalRead(SW_LIMIT_D_PIN);
	LIMIT_L = digitalRead(SW_LIMIT_L_PIN);
}

// CLACK!  Releases tension on the claw string and does a claw check.
void clack() {
	readLimitSwitches();
	moveUD(-1);
	delay(500);
	moveUD(0);
	moveClaw(1);
	delay(100);
	moveClaw(0);
}

// Sets a default speed for all motors.
void setDefaultSpeed() {
	MT_UD.setSpeed(DEFAULT_SPEED_UD);
	MT_FB.setSpeed(DEFAULT_SPEED_FB);
	MT_LR.setSpeed(DEFAULT_SPEED_LR);
	CLAW.setSpeed(DEFAULT_SPEED_CLAW);
}

bool parkAll() {
	readLimitSwitches();

	// Park the claw first so it does not crash into anything while parking the gantry.
	if (!parkClaw()) {
		return false;
	}
	if (!parkGantry()) {
		return false;
	}
	return true;
}

bool parkClaw() {
	unsigned long startMillis = millis();
	unsigned long currentMillis = 0;
	while (!isULimitTriggered()) {
		readLimitSwitches();
		if (!isULimitTriggered()) {
			moveUD(1);
		} else {
			moveUD(0);
		}

		currentMillis = millis();
		if (!isULimitTriggered() && currentMillis - startMillis >= 2000) {
			// Error state, took too long.
			// Don't break, return.  We don't want to attempt parking the gantry.
			return false;
		}
	}
	clawParked = true;
	return true;
}

bool parkGantry() {
	unsigned long startMillis = millis();
	unsigned long currentMillis = 0;
	while (!isFLimitTriggered() || !isLLimitTriggered()) {
		readLimitSwitches();

		if (!isFLimitTriggered()) {
			moveFB(1);
		} else {
			moveFB(0);
		}

		if (!isLLimitTriggered()) {
			moveLR(-1);
		} else {
			moveLR(0);
		}

		currentMillis = millis();
		if ((!isFLimitTriggered() || !isLLimitTriggered()) && currentMillis - startMillis >= 2000) {
			// Error state, took too long.
			return false;
		}
	}
	gantryParked = true;
	return true;
}

bool isFLimitTriggered() {
	return LIMIT_F == 0;
}

bool isBLimitTriggered() {
	return LIMIT_B == 0;
}

bool isLLimitTriggered() {
	return LIMIT_L == 0;
}

bool isULimitTriggered() {
	return LIMIT_U == 0;
}

bool isDLimitTriggered() {
	return LIMIT_D == 0;
}

// 1 = Forwards towards the player.
// 0 = Stop
//-1 = Backwards away from the player.
void moveFB(int dir) {
	switch (dir) {
		case 1: // Forward
			if (!isFLimitTriggered()) {
				MT_FB.run(BACKWARD); // I'm not swapping the wires.  *Angry elf noises.*
			}
			break;
		case -1: // Backward
			if (!isBLimitTriggered()) {
				MT_FB.run(FORWARD);
			}
			break;
		case 0: // Stop
		default:
			MT_FB.run(RELEASE);
			break;
	}
}

// 1 = Right towards the right side of the machine.
// 0 = Stop
//-1 = Left towards the left side of the machine.
void moveLR(int dir) {
	switch (dir) {
		case 1: // Right
			// No limit switch installed.
			MT_LR.run(BACKWARD);
			break;
		case -1: // Left
			if (!isLLimitTriggered()) {
				MT_LR.run(FORWARD);
			}
			break;
		case 0: // Stop
		default:
			MT_LR.run(RELEASE);
			break;
	}
}

// 1 = Up
// 0 = Stop
//-1 = Down
void moveUD(int dir) {
	switch (dir) {
		case 1: // Up
			if (!isULimitTriggered()) {
				MT_UD.run(BACKWARD);
			}
			break;
		case -1: // Down
			if (!isDLimitTriggered()) {
				MT_UD.run(FORWARD);
			}
			break;
		case 0: // Stop
		default:
			MT_UD.run(RELEASE);
			break;
	}
}

// 1 = Close
// 0 = Open
void moveClaw(int state) {
	// Forward and backward closes the claw, but forward(in this wiring configuration) has the best holding force.
	switch (state) {
		case 1: // Close
			CLAW.run(FORWARD);
			break;
		case 0: // Stop
		default:
			CLAW.run(RELEASE);
			break;
	}
}