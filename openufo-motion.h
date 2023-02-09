#include "config.h"
#include <L298N.h>

// Limit Switch Status
bool LIMIT_F = false;
bool LIMIT_B = false;
bool LIMIT_L = false;
bool LIMIT_R = false;
bool LIMIT_U = false;
bool LIMIT_D = false;
byte limitSwitchState = 0b00000000;

// Player Joystick Status
bool PLAYER_F = false;
bool PLAYER_B = false;
bool PLAYER_L = false;
bool PLAYER_R = false;
bool PLAYER_D = false;
byte playerSwitchState = 0b00000000;

// Interal Switch Status
bool SW_TOKEN_CREDIT_PRESSED = false;
bool SW_SERVICE_CREDIT_PRESSED = false;
bool SW_PROGRAM_PRESSED = false;
bool SW_TILT_PRESSED = false;
bool SW_PRIZE_DETECTED = false;
byte internalSwitchState = 0b00000000;

// Motor Setup
L298N MT_UD(MT_UD_E_PIN, MT_UD_1_PIN, MT_UD_2_PIN);
L298N MT_FB(MT_FB_E_PIN, MT_FB_1_PIN, MT_FB_2_PIN);
L298N MT_LR(MT_LR_E_PIN, MT_LR_1_PIN, MT_LR_2_PIN);

byte gantryFBSpeed = DEFAULT_SPEED_FB;
byte gantryLRSpeed = DEFAULT_SPEED_LR;
byte gantryUDSpeed = DEFAULT_SPEED_UD;
byte clawStrength = DEFAULT_STRENGTH_CLAW;

// Switch setup for INPUT_PULLUP.
#define NUM_SW 16
const uint8_t SW_PINS[NUM_SW] = {SW_LIMIT_F_PIN, SW_LIMIT_B_PIN, SW_LIMIT_L_PIN, SW_LIMIT_R_PIN, SW_LIMIT_U_PIN, SW_LIMIT_D_PIN, SW_DIR_F_PIN, SW_DIR_B_PIN, SW_DIR_L_PIN, SW_DIR_R_PIN, SW_DIR_D_PIN, SW_TOKEN_CREDIT_PIN, SW_SERVICE_CREDIT_PIN, SW_PROGRAM_PIN, SW_TILT_PIN, SW_PRIZE_DETECT_PIN};

// State machine states.
#define STATE_PROGRAM -3
#define STATE_ERROR -2
#define STATE_BOOT -1
#define STATE_PARKED_ATTRACT 0
#define STATE_PARKED_CREDITS 1
#define STATE_PLAYER_CONTROL 2
#define STATE_GRAB 3
#define STATE_GRAB_PARK 4
#define STATE_PRIZE_DETECT 5

int currentState = -1;

// Gantry helpers.
// These are prefixed with G_ for gantry since the AFMotor library uses FORWARD and BACKWARD.
#define G_FORWARD 1
#define G_BACKWARD -1
#define G_LEFT -1
#define G_RIGHT 1
#define G_UP 1
#define G_DOWN -1
#define G_STOP 0

struct gantryMove {
	int fb;
	int lr;
	int ud;
};

gantryMove currentGantryMove = {G_STOP, G_STOP, G_STOP};
byte lastGantryMove = 0b00000000;

unsigned long stopMoveAt = 0;

// Drop button LED state and millisecond tracker for all LEDs.
//  0 - Off
//  1 - Flashing
//  2 - Solid
int dropButtonLEDState = 0;

// Use these millisecond tracker for any LEDs that need to flash to keep them in unison.
unsigned long flashLEDLastMillis = 0;
unsigned long flashLEDCurrentState = LOW;

unsigned long prizeDetectStartMillis = 0;

// Credit Handling
uint16_t totalCredits = 0;
unsigned long creditDetectStartMillis = 0; // The coin comparitor sends out a 25/50/100ms pulse so we have to time to prevent detecting a long pulse as multiple credits.
byte creditDetectPulseMs = 100;			   // Set this to match the coin comparitor pulse.
bool creditSwitchPressed = false;

// Play time out.
//  0 - Infinite
//  x - Milliseconds
unsigned long playTimeLimit = DEFAULT_PLAY_TIME_LIMIT;
unsigned long playStartMillis = 0;