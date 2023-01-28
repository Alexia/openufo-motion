// Limit Switches
#define SW_LIMIT_F_PIN 47
#define SW_LIMIT_B_PIN 49
#define SW_LIMIT_U_PIN 45
#define SW_LIMIT_D_PIN 53
#define SW_LIMIT_L_PIN 51

// Motor Speeds
#define PARKING_SPEED 255
#define DEFAULT_SPEED_FB 255 // 0-255
#define DEFAULT_SPEED_LR 255 // 0-255
#define DEFAULT_SPEED_UD 255 // 0-255
// 153(60%) is the recommended minimum for 24V motors.
#define MINIMUM_SPEED 153

#define DEFAULT_STRENGTH_CLAW 255 // 0-255

// Player Input
#define SW_DIR_F_PIN 48
#define SW_DIR_B_PIN 46
#define SW_DIR_L_PIN 50
#define SW_DIR_R_PIN 52
#define SW_DIR_D_PIN 44

// Credits
#define SW_TOKEN_CREDIT_PIN 42

// Service Buttons
#define SW_SERVICE_CREDIT_PIN 40
#define SERVICE_CREDIT_IS_REGULAR false // Set this to true and SW_SERVICE_CREDIT_PIN = SW_TOKEN_CREDIT_PIN if both the coin mechanism and service credit button are wired to the same pin.
#define SW_PROGRAM_PIN 38

// Lights
#define LED_ERROR_PIN LED_BUILTIN
#define LED_DROP_BUTTON_PIN 23