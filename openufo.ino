#include "openufo.h"

void setup() {
	initSwitches();
	initLights();
	initClaw();

	startCom();
	setParkingSpeed();
	delay(1000); // Give time for all boards to boot and setup communication.

	clack();

	if (parkAll()) {
		changeState(STATE_PARKED_ATTRACT);
	} else {
		changeState(STATE_ERROR);
	}
	setDefaultSpeed();
}

void initSwitches() {
	for (int i = 0; i < NUM_SW; i++) {
		pinMode(SW_PINS[i], INPUT_PULLUP);
	}
	updateSwitches();
}

void initLights() {
	pinMode(LED_ERROR_PIN, OUTPUT);
	digitalWrite(LED_ERROR_PIN, LOW);

	pinMode(LED_DROP_BUTTON_PIN, OUTPUT);
	digitalWrite(LED_DROP_BUTTON_PIN, LOW);
}

void initClaw() {
	pinMode(CLAW_PWM_PIN, OUTPUT);
	analogWrite(CLAW_PWM_PIN, 0);
}

void updateLEDLastMillis() {
	if (millis() - flashLEDLastMillis >= 500) {
		flashLEDLastMillis = millis();
		flashLEDCurrentState = !flashLEDCurrentState;
	}
}

void updateCredits() {
	if (totalCredits < 0) {
		totalCredits = 0;
	}

	if ((SW_TOKEN_CREDIT_PRESSED || SW_SERVICE_CREDIT_PRESSED) && creditDetectStartMillis == 0) {
		totalCredits++;
		sendCom("cred", (String)totalCredits);
	}

	if (creditDetectStartMillis == 0) {
		creditDetectStartMillis = millis();
	}
	if (millis() - creditDetectStartMillis >= creditDetectPulseMs) {
		creditDetectStartMillis = 0;
	}
}

void doUpdates() {
	updateSwitches();
	updateGantryMove(); // MUST HAPPEN OUTSIDE OF THE STATES!  Otherwise moves or stops may never occur.
	updateLEDLastMillis();

	updateCredits(); // Must happen after updateSwitches();
}

void loop() {
	doUpdates();

	// TOOD:
	// Serial Communication
	// Settings/EEPROM
	// Fix/rework parking to not block the main loop.  This causes stuff like credit detection to fail.

	// States:
	// Boot
	// (Loop ->)
	// Parked, Attract Mode
	// Parked, Credit(s)
	// Moving Gantry (Player has control)
	// Grabbing (Player loses control)
	// Prize Detection (Detect/Timeout)
	// (<- Loop)
	switch (currentState) {
		case STATE_PROGRAM:
			// Programming mode blocks all other states until it exits.
		case STATE_BOOT:
			// This state should never be reached after a normal boot so just fall through to STATE_ERROR.
		case STATE_ERROR:
			// If we are in an error state or failed boot prevent all operations until powered off.
			// This is intended for safety to prevent damage to the machine and operator.
			while (1) {
				digitalWrite(LED_ERROR_PIN, HIGH);
				delayWithUpdates(500);
				digitalWrite(LED_ERROR_PIN, LOW);
				delayWithUpdates(500);
			}
			break;
		case STATE_PARKED_ATTRACT:
			// If credits exist, short circuit and transition to STATE_PARKED_CREDITS.
			// Drop button LED is off.
			if (dropButtonLEDState != 0) {
				digitalWrite(LED_DROP_BUTTON_PIN, LOW);
				dropButtonLEDState = 0;
			}

			// Input does nothing.
			// Adding credit triggers transition to STATE_PARKED_CREDITS.

			if (totalCredits > 0) {
				parkAll(); // This was originally placed here to get around the L293D motor controller crapping out.  This can still help with failure conditions though.
				changeState(STATE_PARKED_CREDITS);
			}
			break;
		case STATE_PARKED_CREDITS:
			// Drop button LED is flashing.
			if (dropButtonLEDState != 1) {
				dropButtonLEDState = 1;
			} else {
				digitalWrite(LED_DROP_BUTTON_PIN, flashLEDCurrentState);
			}

			// Input allowed from joystick only; triggers transition to STATE_PLAYER_CONTROL.
			// TODO: This should only transition when moving away from the parked position.  Some machines start from the right.
			if (PLAYER_F || PLAYER_B || PLAYER_L || PLAYER_R) {
				totalCredits--;
				sendCom("cred", (String)totalCredits);
				playStartMillis = millis();
				changeState(STATE_PLAYER_CONTROL);
			}
			break;
		case STATE_PLAYER_CONTROL:
			// TODO: Play timer begins.
			// Drop button LED is solid.
			if (dropButtonLEDState != 2) {
				digitalWrite(LED_DROP_BUTTON_PIN, HIGH);
				dropButtonLEDState = 2;
			}

			// Drop button input triggers transition to STATE_GRABBING.
			if (PLAYER_D || millis() - playStartMillis >= playTimeLimit) {
				playStartMillis = 0;
				currentGantryMove.fb = G_STOP;
				currentGantryMove.lr = G_STOP;
				changeState(STATE_GRAB);
			}

			if (PLAYER_F) {
				currentGantryMove.fb = G_FORWARD;
			} else if (PLAYER_B) {
				currentGantryMove.fb = G_BACKWARD;
			} else {
				currentGantryMove.fb = G_STOP;
			}
			if (PLAYER_L) {
				currentGantryMove.lr = G_LEFT;
			} else if (PLAYER_R) {
				currentGantryMove.lr = G_RIGHT;
			} else {
				currentGantryMove.lr = G_STOP;
			}
			break;
		case STATE_GRAB:
			// Player loses all control.
			// Drop button LED is off.
			if (dropButtonLEDState != 0) {
				digitalWrite(LED_DROP_BUTTON_PIN, LOW);
				dropButtonLEDState = 0;
			}

			doGrab();

			changeState(STATE_GRAB_PARK);
			break;
		case STATE_GRAB_PARK:
			if (parkAll()) {
				changeState(STATE_PRIZE_DETECT);
			} else {
				changeState(STATE_ERROR);
			}
			moveClaw(0);
			break;
		case STATE_PRIZE_DETECT:
			if (prizeDetectStartMillis == 0) {
				prizeDetectStartMillis = millis();
			}
			if (isPrizeDetected()) {
				prizeDetectStartMillis = 0;
				changeState(STATE_PARKED_ATTRACT);
				// Send serial event of prize detected.
				sendCom("prde", "1");
			} else {
				if (millis() - prizeDetectStartMillis >= 2000) {
					prizeDetectStartMillis = 0;
					changeState(STATE_PARKED_ATTRACT);
					// Send serial event of prize undetected.
					sendCom("prde", "0");
				}
			}
			break;
		default:
			break;
	}
}

void changeState(int state) {
	currentState = state;
	sendCom("stat", (String)state);
}

void sendCom(String word, String data) {
	Serial.println(word + ":" + data);
}

void startCom() {
	Serial.begin(115200);
	com.begin(Serial);
}

void updateSwitches() {
	readPlayerSwitches();
	readLimitSwitches();
	readInternalSwitches();
}

void readPlayerSwitches() {
	PLAYER_F = !digitalRead(SW_DIR_F_PIN);
	PLAYER_B = !digitalRead(SW_DIR_B_PIN);
	PLAYER_L = !digitalRead(SW_DIR_L_PIN);
	PLAYER_R = !digitalRead(SW_DIR_R_PIN);
	PLAYER_D = !digitalRead(SW_DIR_D_PIN);

	byte newState = 0b00000000;
	bitWrite(newState, 7, PLAYER_F);
	bitWrite(newState, 6, PLAYER_B);
	bitWrite(newState, 5, PLAYER_L);
	bitWrite(newState, 4, PLAYER_R); // Reserved
	bitWrite(newState, 3, 0);		 // Reserved
	bitWrite(newState, 2, PLAYER_D);
	bitWrite(newState, 1, 0); // Reserved
	bitWrite(newState, 0, 0); // Reserved

	if (newState != playerSwitchState) {
		playerSwitchState = newState;
		sendCom("plsw", (String)playerSwitchState);
	}
}

void readLimitSwitches() {
	LIMIT_F = !digitalRead(SW_LIMIT_F_PIN);
	LIMIT_B = !digitalRead(SW_LIMIT_B_PIN);
	LIMIT_U = !digitalRead(SW_LIMIT_U_PIN);
	LIMIT_D = !digitalRead(SW_LIMIT_D_PIN);
	LIMIT_L = !digitalRead(SW_LIMIT_L_PIN);

	byte newState = 0b00000000;
	bitWrite(newState, 7, LIMIT_F);
	bitWrite(newState, 6, LIMIT_B);
	bitWrite(newState, 5, LIMIT_L);
	bitWrite(newState, 4, 0); // Reserved - LIMIT_R
	bitWrite(newState, 3, LIMIT_U);
	bitWrite(newState, 2, LIMIT_D);
	bitWrite(newState, 1, 0); // Reserved - Claw Open
	bitWrite(newState, 0, 0); // Reserved - Claw Close

	if (newState != limitSwitchState) {
		limitSwitchState = newState;
		sendCom("lisw", (String)limitSwitchState);
	}
}

void readInternalSwitches() {
	SW_TOKEN_CREDIT_PRESSED = !digitalRead(SW_TOKEN_CREDIT_PIN);
	SW_SERVICE_CREDIT_PRESSED = !digitalRead(SW_SERVICE_CREDIT_PIN);
	SW_PROGRAM_PRESSED = !digitalRead(SW_PROGRAM_PIN);
	SW_TILT_PRESSED = !digitalRead(SW_TILT_PIN);
	SW_PRIZE_DETECTED = digitalRead(SW_PRIZE_DETECT_PIN);

	byte newState = 0b00000000;
	bitWrite(newState, 7, SW_TOKEN_CREDIT_PRESSED);
	bitWrite(newState, 6, SW_SERVICE_CREDIT_PRESSED);
	bitWrite(newState, 5, SW_PROGRAM_PRESSED);
	bitWrite(newState, 4, SW_TILT_PRESSED);
	bitWrite(newState, 3, SW_PRIZE_DETECTED);
	bitWrite(newState, 2, 0); // Reserved
	bitWrite(newState, 1, 0); // Reserved
	bitWrite(newState, 0, 0); // Reserved

	if (newState != internalSwitchState) {
		internalSwitchState = newState;
		sendCom("insw", (String)internalSwitchState);
	}
}

bool isFLimitTriggered() {
	return LIMIT_F;
}

bool isBLimitTriggered() {
	return LIMIT_B;
}

bool isLLimitTriggered() {
	return LIMIT_L;
}

bool isULimitTriggered() {
	return LIMIT_U;
}

bool isDLimitTriggered() {
	return LIMIT_D;
}

bool isPrizeDetected() {
	return SW_PRIZE_DETECTED;
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
	delay(500);
}

// Sets a default speed for all motors.
void setDefaultSpeed() {
	MT_UD.setSpeed(DEFAULT_SPEED_UD);
	MT_FB.setSpeed(DEFAULT_SPEED_FB);
	MT_LR.setSpeed(DEFAULT_SPEED_LR);
}

// Sets a parking speed for all motors.
void setParkingSpeed() {
	MT_UD.setSpeed(PARKING_SPEED);
	MT_FB.setSpeed(PARKING_SPEED);
	MT_LR.setSpeed(PARKING_SPEED);
}

bool isAllParked() {
	return isGantryParked() && isClawParked();
}

bool isClawParked() {
	readLimitSwitches();
	return isULimitTriggered();
}

bool isGantryParked() {
	readLimitSwitches();
	return isFLimitTriggered() && isLLimitTriggered();
}

void delayWithUpdates(unsigned long milliseconds) {
	unsigned long startMillis = millis();
	while (millis() - startMillis < milliseconds) {
		doUpdates();
	}
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
	if (isClawParked()) {
		// Short circuit if the claw is parked.
		return true;
	}

	sendCom("clpa", "");

	unsigned long startMillis = millis();
	unsigned long currentMillis = startMillis;

	while (!isClawParked() && currentMillis - startMillis < 6000) {
		currentMillis = millis();
		doUpdates(); // If we are blocking the main loop we have to call doUpdates().

		if (!isClawParked()) {
			moveUD(1);
		}

		if (isClawParked()) {
			break;
		}
	}

	moveUD(0);
	delayWithUpdates(250); // Shitty debounce.

	if (isClawParked()) {
		return true;
	}

	Serial.println("Claw parking failed.");
	// Error state, took too long.
	return false;
}

bool parkGantry() {
	if (isGantryParked()) {
		// Short circuit if the gantry is parked.
		return true;
	}

	sendCom("gapa", "");

	unsigned long startMillis = millis();
	unsigned long currentMillis = startMillis;

	currentGantryMove.fb = G_FORWARD;
	currentGantryMove.lr = G_LEFT;

	while (!isGantryParked() && currentMillis - startMillis < 4000) {
		currentMillis = millis();
		doUpdates(); // If we are blocking the main loop we have to call doUpdates().

		if (isGantryParked()) {
			break;
		}
	}

	currentGantryMove.fb = G_STOP;
	currentGantryMove.lr = G_STOP;
	updateGantryMove();
	delayWithUpdates(250); // Also shitty debounce here.

	if (isGantryParked()) {
		return true;
	}

	emergencyStop();
	currentGantryMove.fb = G_STOP;
	currentGantryMove.lr = G_STOP;

	Serial.println("Gantry parking failed.");
	// Error state, took too long.
	return false;
}

void updateGantryMove() {
	moveFB(currentGantryMove.fb);
	moveLR(currentGantryMove.lr);
}

void emergencyStop() {
	// Use an explicit stop that bypasses any code fluff.
	MT_FB.run(RELEASE);
	MT_LR.run(RELEASE);
	MT_UD.run(RELEASE);
}

// 1 = Forwards towards the player.
// 0 = Stop
//-1 = Backwards away from the player.
void moveFB(int dir) {
	readLimitSwitches();
	switch (dir) {
		case 1: // Forward
			if (!isFLimitTriggered()) {
				MT_FB.run(BACKWARD); // I'm not swapping the wires.  *Angry elf noises.*
			} else {
				currentGantryMove.fb = 0;
				MT_FB.run(RELEASE);
			}
			break;
		case -1: // Backward
			if (!isBLimitTriggered()) {
				MT_FB.run(FORWARD);
			} else {
				currentGantryMove.fb = 0;
				MT_FB.run(RELEASE);
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
	readLimitSwitches();
	switch (dir) {
		case 1: // Right
			// No limit switch installed.
			MT_LR.run(BACKWARD);
			break;
		case -1: // Left
			if (!isLLimitTriggered()) {
				MT_LR.run(FORWARD);
			} else {
				currentGantryMove.lr = 0;
				MT_LR.run(RELEASE);
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
	readLimitSwitches();
	switch (dir) {
		case 1: // Up
			if (!isULimitTriggered()) {
				MT_UD.run(BACKWARD);
			} else {
				MT_UD.run(RELEASE);
			}
			break;
		case -1: // Down
			if (!isDLimitTriggered()) {
				MT_UD.run(FORWARD);
			} else {
				MT_UD.run(RELEASE);
			}
			break;
		case 0: // Stop
		default:
			MT_UD.run(RELEASE);
			break;
	}
}

void doGrab() {
	unsigned long startMillis = millis();
	unsigned long currentMillis = startMillis;

	while (!isDLimitTriggered() && currentMillis - startMillis < 2000) {
		currentMillis = millis();
		doUpdates(); // If we are blocking the main loop we have to call doUpdates().

		if (!isDLimitTriggered()) {
			moveUD(-1);
		}

		if (isDLimitTriggered()) {
			break;
		}
	}

	moveUD(0);
	moveClaw(1);
}

// 1 = Close
// 0 = Open
void moveClaw(int state) {
	switch (state) {
		case 1: // Close
			analogWrite(CLAW_PWM_PIN, DEFAULT_STRENGTH_CLAW);
			break;
		case 0: // Stop
		default:
			analogWrite(CLAW_PWM_PIN, 0);
			break;
	}
}