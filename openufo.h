#include "config.h"
#include <AFMotor.h>
#include <SerialTransfer.h>

int LIMIT_F = 0;
int LIMIT_B = 0;
int LIMIT_U = 0;
int LIMIT_D = 0;
int LIMIT_L = 0;

int PLAYER_F = 0;
int PLAYER_B = 0;
int PLAYER_L = 0;
int PLAYER_R = 0;
int PLAYER_D = 0;

AF_DCMotor MT_UD(1, MOTOR12_64KHZ);
AF_DCMotor CLAW(2, MOTOR12_64KHZ);
AF_DCMotor MT_FB(3, MOTOR34_64KHZ);
AF_DCMotor MT_LR(4, MOTOR34_64KHZ);

SerialTransfer com;

#define NUM_SW 13
const uint8_t SW_PINS[NUM_SW] = {SW_LIMIT_F_PIN, SW_LIMIT_B_PIN, SW_LIMIT_U_PIN, SW_LIMIT_D_PIN, SW_LIMIT_L_PIN, SW_DIR_F_PIN, SW_DIR_B_PIN, SW_DIR_L_PIN, SW_DIR_R_PIN, SW_DIR_D_PIN, SW_TOKEN_CREDIT_PIN, SW_SERVICE_CREDIT_PIN, SW_PROGRAM_PIN};

#define STATE_PROGRAM -3
#define STATE_ERROR -2
#define STATE_BOOT -1
#define STATE_PARKED_ATTRACT 0
#define STATE_PARKED_CREDITS 1
#define STATE_PLAYER_CONTROL 2
#define STATE_GRABBING 3
#define STATE_PRIZE_DETECT 4

int currentState = -1;

// Gantry helpers.
// These are prefixed with G_ for gantry since the AFMotor library uses FORWARD and BACKWARD.
#define G_FORWARD 1
#define G_BACKWARD -1
#define G_LEFT -1
#define G_RIGHT 1
#define G_STOP 0

struct gantryMove {
	int fb;
	int lr;
};

gantryMove currentGantryMove = {G_STOP, G_STOP};

// 0 - Off
// 1 - Flashing
// 2 - Solid
int dropButtonLEDState = 0;

// Use these millisecond tracker for any LEDs that need to flash to keep them in unison.
unsigned long flashLEDLastMillis = 0;
unsigned long flashLEDCurrentState = LOW;