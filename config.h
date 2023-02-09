#define OPENUFO_VERSION "0.0.1"

// Motor_Directions_Function_PIN
#define MT_FB_E_PIN 8
#define MT_FB_1_PIN 10
#define MT_FB_2_PIN 9
#define MT_LR_E_PIN 13
#define MT_LR_1_PIN 11
#define MT_LR_2_PIN 12
#define MT_UD_E_PIN 7
#define MT_UD_1_PIN 5
#define MT_UD_2_PIN 6

// Limit Switches
#define SW_LIMIT_F_PIN 47
#define SW_LIMIT_B_PIN 49
#define SW_LIMIT_U_PIN 45
#define SW_LIMIT_D_PIN 53
#define SW_LIMIT_L_PIN 51
#define SW_LIMIT_R_PIN 42

// Motor Speeds
#define PARKING_SPEED 255
#define DEFAULT_SPEED_FB 255 // 0-255
#define DEFAULT_SPEED_LR 255 // 0-255
#define DEFAULT_SPEED_UD 255 // 0-255
// 153(60%) is the recommended minimum for 24V motors with a L293D motor controller.
#define MINIMUM_SPEED 153

#define CLAW_PWM_PIN 3
#define DEFAULT_STRENGTH_CLAW 255 // 0-255
// 40: Low win rate(if at all) for 50mm gacha capsules.  They fall out of the claw near the top.
// 45: 95% win rate for 50mm gacha capsules.
// 55: 100% win rate for 50mm gacha capsules.  Chibi plushie falls out.
// 127-255: The claw becomes a monster and can pick up anything.

// Player Input
#define SW_DIR_F_PIN 48
#define SW_DIR_B_PIN 46
#define SW_DIR_L_PIN 50
#define SW_DIR_R_PIN 52
#define SW_DIR_D_PIN 44

// Credits
#define SW_TOKEN_CREDIT_PIN 41

// Service Buttons
#define SW_SERVICE_CREDIT_PIN PIN_A4
#define SERVICE_CREDIT_IS_REGULAR false // Set this to true and SW_SERVICE_CREDIT_PIN = SW_TOKEN_CREDIT_PIN if both the coin mechanism and service credit button are wired to the same pin.
#define SW_PROGRAM_PIN PIN_A3

// Lights
#define LED_ERROR_PIN LED_BUILTIN
#define LED_DROP_BUTTON_PIN 43

// Tilt Switch
#define SW_TILT_PIN PIN_A5

// Prize Detect
#define SW_PRIZE_DETECT_PIN 39

// The maximum amount in time in milliseconds the claw can descent towards the prize floor before stopping.
#define GRAB_DESCENT_TIME_MAX_MS 2500

// Amount of milliseconds to pause after closing the claw at the bottom of the grab descent.
#define CLAW_DWELL_MS 500

// Play time out.
//  0 - Infinite
//  x - Milliseconds
#define DEFAULT_PLAY_TIME_LIMIT 60000