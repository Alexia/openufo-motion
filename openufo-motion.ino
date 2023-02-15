#include "openufo-motion.h"

void setup() {
	initSwitches();
	initLights();
	initClaw();

	startCom();
	setParkingSpeed();
	delay(1000); // Give time for all boards to boot and setup communication.

	clack();

	if (parkAll()) {
		changeState(STATE_BOOT);
	} else {
		changeState(STATE_ERROR);
	}
	setGantrySpeed();
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

void doUpdates() {
	updateSwitches();
	updateCredits();	// Must happen after updateSwitches();
	timedMove();		// Must happen before updateGantryMove();
	updateGantryMove(); // MUST HAPPEN OUTSIDE OF THE STATES!  Otherwise moves or stops may never occur.
	updateLEDLastMillis();
	readCom(); // This goes last.
}

void loop() {
	doUpdates();

	// TODO:
	// Serial Communication (In Progress)
	// Settings/EEPROM
	// Fix/rework parking to not block the main loop.  This causes stuff like credit detection to fail.
	// Claw as part of movement/current move.

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
			break;
		case STATE_BOOT:
			// Tell any listeners the motion controller version.
			sendCom("vers", OPENUFO_VERSION);
			// Load EEPROM Settings
			// loadEEPROMSettings();
			changeState(STATE_PARKED_ATTRACT);
			break;
		case STATE_ERROR:
			// If we are in an error state or failed boot prevent all operations until powered off.
			// This is intended for safety to prevent damage to the machine and operator.
			while (1) {
				digitalWrite(LED_ERROR_PIN, HIGH);
				delayWithUpdates(500);
				digitalWrite(LED_ERROR_PIN, LOW);
				delayWithUpdates(500);
				doUpdates();
				if (currentState != STATE_ERROR) {
					// Error state was cleared with the cler short word.
					break;
				}
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

			if (totalCredits == 0) {
				// If credits are removed by service mode or the controller then go back to attract mode.
				changeState(STATE_PARKED_ATTRACT);
				break;
			}

			// Input allowed from joystick only; triggers transition to STATE_PLAYER_CONTROL.
			// TODO: This should only transition when moving away from the parked position.  Some machines start from the right.
			if (PLAYER_F || PLAYER_B || PLAYER_L || PLAYER_R) {
				givePlayerControl();
			}
			break;
		case STATE_PLAYER_CONTROL:
			// TODO: Play timer begins.
			// Drop button LED is solid.
			if (dropButtonLEDState != 2) {
				digitalWrite(LED_DROP_BUTTON_PIN, HIGH);
				dropButtonLEDState = 2;
			}

			// Ignore player switch input if a timed move is in progress.
			if (stopMoveAt == 0) {
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
			}

			// Drop button input triggers transition to STATE_GRABBING.
			if (PLAYER_D || millis() - playStartMillis >= playTimeLimit) {
				playStartMillis = 0;
				currentGantryMove.fb = G_STOP;
				currentGantryMove.lr = G_STOP;
				changeState(STATE_GRAB);
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

void updateLEDLastMillis() {
	if (millis() - flashLEDLastMillis >= 500) {
		flashLEDLastMillis = millis();
		flashLEDCurrentState = !flashLEDCurrentState;
	}
}

void updateCredits() {
	if ((SW_TOKEN_CREDIT_PRESSED || SW_SERVICE_CREDIT_PRESSED) && creditDetectStartMillis == 0) {
		addCredit(1);
	}

	if (creditDetectStartMillis == 0) {
		creditDetectStartMillis = millis();
	}
	if (millis() - creditDetectStartMillis >= creditDetectPulseMs) {
		creditDetectStartMillis = 0;
	}
}

void addCredit(uint16_t amount) {
	if (totalCredits + amount >= UINT16_MAX || (totalCredits + amount) < totalCredits) {
		totalCredits = UINT16_MAX;
	} else {
		totalCredits += amount;
	}
	sendCom("cred", (String)totalCredits);
}

void subtractCredit(uint16_t amount) {
	if ((totalCredits - amount) <= 0 || (totalCredits - amount) > totalCredits) {
		totalCredits = 0;
	} else {
		totalCredits -= amount;
	}
	sendCom("cred", (String)totalCredits);
}

void changeState(int state) {
	currentState = state;
	sendCom("stat", (String)currentState);
}

void givePlayerControl() {
	subtractCredit(1);
	playStartMillis = millis();
	sendCom("time", (String)(playTimeLimit / 1000));
	changeState(STATE_PLAYER_CONTROL);
}

void startCom() {
	Serial.begin(115200);
}

void sendCom(String word, String data) {
	Serial.println(word + ":" + data);
}

void readCom() {
	String data;
	if (Serial.available() > 0) {
		data = Serial.readStringUntil('\n');
		int splitPos = data.indexOf(":");
		if (splitPos > 0) {
			String shortWord = data.substring(0, splitPos);
			String parameters = data.substring(splitPos + 1);
			processCommands(shortWord, parameters);
		}
	}
}

void processCommands(String shortWord, String parameters) {
	if (shortWord == "set") {
		bool set = false;
		int splitPos = parameters.indexOf(":");
		if (splitPos > 0) {
			String setting = parameters.substring(0, splitPos);
			String value = parameters.substring(splitPos + 1);

			// Claw Strength
			if (setting == "clst") {
				clawStrength = constrain(value.toInt(), 0, 255);
				set = true;
			}

			// Gantry FB Speed
			if (setting == "spfb") {
				gantryFBSpeed = constrain(value.toInt(), MINIMUM_SPEED, 255);
				setGantrySpeed();
				set = true;
			}

			// Gantry LR Speed
			if (setting == "splr") {
				gantryLRSpeed = constrain(value.toInt(), MINIMUM_SPEED, 255);
				setGantrySpeed();
				set = true;
			}

			// Gantry UD Speed
			if (setting == "spud") {
				gantryUDSpeed = constrain(value.toInt(), MINIMUM_SPEED, 255);
				setGantrySpeed();
				set = true;
			}

			// Gantry UD Speed
			if (setting == "time") {
				playTimeLimit = constrain(value.toInt(), 10, 120) * 1000;
				set = true;
			}
		}

		if (set) {
			sendCom("ack", shortWord);
		} else {
			sendCom("ack", "fail");
		}
	}

	if (shortWord == "move") {
		int splitPos = parameters.indexOf(":");
		String direction;
		String millisString;
		unsigned long milliseconds;
		bool timedMove = false;
		if (splitPos > 0) {
			direction = parameters.substring(0, splitPos);
			millisString = parameters.substring(splitPos + 1);
			milliseconds = millisString.toInt();
			milliseconds = constrain(milliseconds, 0, 1000);
			timedMove = true;
		} else {
			direction = parameters;
		}
		if (direction.length() == 3) {
			String fb = direction.substring(0, 1);
			String lr = direction.substring(1, 2);
			String ud = direction.substring(2, 3);

			if (fb != "n") {
				if (fb == "f") {
					currentGantryMove.fb = G_FORWARD;
				} else if (fb == "b") {
					currentGantryMove.fb = G_BACKWARD;
				} else {
					currentGantryMove.fb = G_STOP;
				}
			}

			if (lr != "n") {
				if (lr == "l") {
					currentGantryMove.lr = G_LEFT;
				} else if (lr == "r") {
					currentGantryMove.lr = G_RIGHT;
				} else {
					currentGantryMove.lr = G_STOP;
				}
			}

			if (ud != "n") {
				if (ud == "u") {
					currentGantryMove.ud = G_UP;
				} else if (ud == "d") {
					// Temporarily disabled until I figure out if it is possible to accidentally unwind the claw string this way.
					// currentGantryMove.ud = G_DOWN;
					currentGantryMove.ud = G_STOP;
				} else {
					currentGantryMove.ud = G_STOP;
				}
			}

			if (timedMove) {
				stopMoveAt = millis() + milliseconds;
			}

			sendCom("ack", shortWord);
		}
	}

	if (shortWord == "cred") {
		bool credSuccess = false;
		if (parameters.length() >= 2) {
			String sign = parameters.substring(0, 1);
			String amount = parameters.substring(1);
			if (sign == "+") {
				addCredit(constrain(amount.toInt(), 0, 100));
				credSuccess = true;
			}
			if (sign == "-") {
				subtractCredit(constrain(amount.toInt(), 0, 100));
				credSuccess = true;
			}
		}
		if (credSuccess) {
			sendCom("ack", shortWord);
		} else {
			sendCom("ack", "fail");
		}
	}

	if (shortWord == "emst") {
		emergencyStop();
		changeState(STATE_ERROR);
		sendCom("ack", shortWord);
	}

	if (shortWord == "cler") {
		changeState(STATE_BOOT);
		sendCom("ack", shortWord);
	}

	if (shortWord == "gapa") {
		parkGantry();
		sendCom("ack", shortWord);
	}

	if (shortWord == "clpa") {
		parkClaw();
		sendCom("ack", shortWord);
	}

	if (shortWord == "claw") {
		if (parameters == "1" || parameters == "0") {
			moveClaw(parameters.toInt());
			sendCom("ack", shortWord);
		} else {
			sendCom("ack", "fail");
		}
	}

	if (shortWord == "stre") {
		sendCom("vers", OPENUFO_VERSION);
		sendCom("stat", (String)currentState);
		sendCom("move", (String)lastGantryMove);
		sendCom("plsw", (String)playerSwitchState);
		sendCom("lisw", (String)limitSwitchState);
		sendCom("insw", (String)internalSwitchState);
		sendCom("cred", (String)totalCredits);
		sendCom("time", "0"); // TODO: Fix me.
		sendCom("ack", shortWord);
	}

	if (shortWord == "plst") {
		if (currentState == STATE_PARKED_CREDITS) {
			givePlayerControl();
		} else {
			sendCom("ack", "fail");
		}
	}
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
	bitWrite(newState, 4, PLAYER_R);
	bitWrite(newState, 3, 0); // Reserved
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
	LIMIT_L = !digitalRead(SW_LIMIT_L_PIN);
	LIMIT_R = !digitalRead(SW_LIMIT_R_PIN);
	LIMIT_U = !digitalRead(SW_LIMIT_U_PIN);
	LIMIT_D = !digitalRead(SW_LIMIT_D_PIN);

	byte newState = 0b00000000;
	bitWrite(newState, 7, LIMIT_F);
	bitWrite(newState, 6, LIMIT_B);
	bitWrite(newState, 5, LIMIT_L);
	bitWrite(newState, 4, LIMIT_R);
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

bool isRLimitTriggered() {
	return LIMIT_R;
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

// Set the defined speed for all motors.
void setGantrySpeed() {
	MT_FB.setSpeed(gantryFBSpeed);
	MT_LR.setSpeed(gantryLRSpeed);
	MT_UD.setSpeed(gantryUDSpeed);
	sendCom("spfb", (String)gantryFBSpeed);
	sendCom("splr", (String)gantryLRSpeed);
	sendCom("spud", (String)gantryUDSpeed);
}

// Sets a parking speed for all motors.
void setParkingSpeed() {
	MT_FB.setSpeed(PARKING_SPEED);
	MT_LR.setSpeed(PARKING_SPEED);
	MT_UD.setSpeed(PARKING_SPEED);
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

	currentGantryMove.ud = G_UP;

	while (!isClawParked() && currentMillis - startMillis < 6000) {
		currentMillis = millis();
		doUpdates(); // If we are blocking the main loop we have to call doUpdates().

		if (isClawParked()) {
			break;
		}
	}

	currentGantryMove.ud = G_STOP;
	updateGantryMove();
	delayWithUpdates(250); // Shitty debounce.

	if (isClawParked()) {
		return true;
	}

	emergencyStop();

	sendCom("erro", "clpa");
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

	sendCom("erro", "gapa");
	// Error state, took too long.
	return false;
}

void updateGantryMove() {
	moveFB(currentGantryMove.fb);
	moveLR(currentGantryMove.lr);
	moveUD(currentGantryMove.ud);

	byte newMove = 0b00000000;
	bitWrite(newMove, 7, currentGantryMove.fb == G_FORWARD);
	bitWrite(newMove, 6, currentGantryMove.fb == G_BACKWARD);
	bitWrite(newMove, 5, currentGantryMove.lr == G_LEFT);
	bitWrite(newMove, 4, currentGantryMove.lr == G_RIGHT);
	bitWrite(newMove, 3, currentGantryMove.ud == G_UP);
	bitWrite(newMove, 2, currentGantryMove.ud == G_DOWN);
	bitWrite(newMove, 1, 0); // Reserved for Claw
	bitWrite(newMove, 0, 0); // Reserved

	if (newMove != lastGantryMove) {
		lastGantryMove = newMove;
		sendCom("move", (String)lastGantryMove);
	}
}

void emergencyStop() {
	// Use an explicit stop that bypasses any code fluff.
	MT_FB.stop();
	MT_LR.stop();
	MT_UD.stop();
	currentGantryMove.fb = G_STOP;
	currentGantryMove.lr = G_STOP;
	currentGantryMove.ud = G_STOP;
	moveClaw(0);
}

// 1 = Forwards towards the player.
// 0 = Stop
//-1 = Backwards away from the player.
void moveFB(int dir) {
	readLimitSwitches();
	switch (dir) {
		case G_FORWARD: // Forward
			if (!isFLimitTriggered()) {
				MT_FB.backward(); // I'm not swapping the wires.  *Angry elf noises.*
			} else {
				currentGantryMove.fb = G_STOP;
				MT_FB.stop();
			}
			break;
		case G_BACKWARD: // Backward
			if (!isBLimitTriggered()) {
				MT_FB.forward();
			} else {
				currentGantryMove.fb = G_STOP;
				MT_FB.stop();
			}
			break;
		case G_STOP: // Stop
		default:
			MT_FB.stop();
			break;
	}
}

// 1 = Right towards the right side of the machine.
// 0 = Stop
//-1 = Left towards the left side of the machine.
void moveLR(int dir) {
	readLimitSwitches();
	switch (dir) {
		case G_RIGHT: // Right
			if (!isRLimitTriggered()) {
				MT_LR.backward();
			} else {
				currentGantryMove.lr = G_STOP;
				MT_LR.stop();
			}
			break;
		case G_LEFT: // Left
			if (!isLLimitTriggered()) {
				MT_LR.forward();
			} else {
				currentGantryMove.lr = G_STOP;
				MT_LR.stop();
			}
			break;
		case G_STOP: // Stop
		default:
			MT_LR.stop();
			break;
	}
}

// 1 = Up
// 0 = Stop
//-1 = Down
void moveUD(int dir) {
	readLimitSwitches();
	switch (dir) {
		case G_UP: // Up
			if (!isULimitTriggered()) {
				MT_UD.backward();
			} else {
				currentGantryMove.ud = G_STOP;
				MT_UD.stop();
			}
			break;
		case G_DOWN: // Down
			if (!isDLimitTriggered()) {
				MT_UD.forward();
			} else {
				currentGantryMove.ud = G_STOP;
				MT_UD.stop();
			}
			break;
		case G_STOP: // Stop
		default:
			MT_UD.stop();
			break;
	}
}

void timedMove() {
	if (stopMoveAt > 0) {
		if (millis() >= stopMoveAt) {
			currentGantryMove.fb = G_STOP;
			currentGantryMove.lr = G_STOP;
			currentGantryMove.ud = G_STOP;
			stopMoveAt = 0;
		}
	}
}

void doGrab() {
	unsigned long startMillis = millis();
	unsigned long currentMillis = startMillis;

	currentGantryMove.ud = G_DOWN;

	while (!isDLimitTriggered() && currentMillis - startMillis < GRAB_DESCENT_TIME_MAX_MS) {
		currentMillis = millis();
		doUpdates(); // If we are blocking the main loop we have to call doUpdates().

		if (isDLimitTriggered()) {
			break;
		}
	}

	currentGantryMove.ud = G_STOP;
	updateGantryMove();

	moveClaw(1);
	delay(CLAW_DWELL_MS); // Short delay to let the claw get into place.
}

// 1 = Close
// 0 = Open
void moveClaw(int state) {
	switch (state) {
		case 1: // Close
			analogWrite(CLAW_PWM_PIN, clawStrength);
			break;
		case 0: // Stop
		default:
			analogWrite(CLAW_PWM_PIN, 0);
			break;
	}
}