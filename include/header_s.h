#include "stb_namespace.h"

String title = "Secret Door";
String versionDate = "12.04.2021";
String version = "version 1.0";

String brainName = String("BrSEC");
String relayCode = String("FPK");
char secret_password[] = "5314";
// I2C Addresses
// Relayboard und OLED
#define RELAY_I2C_ADD 0x38 /* Relay Expander */
#define OLED_ADD 0x3C      /* Ist durch Hardware des OLEDs 0x3C */

// Keypad Addresses
#define KEYPAD_ADD 0x38
// ______________________EINSTELLUNGEN______________________
// RFID

// Relay config
// RELAY
enum REL_PIN {
    REL_0_PIN,     // 0 First room light
    REL_1_PIN,     // 1 Light 2nd room
    REL_2_PIN,     // 2 UV Light
    REL_3_PIN,     // 3 Alarm
    REL_4_PIN,     // 4 Empty
    REL_DOOR_PIN,  // 5 Fireplace valve
    REL_6_PIN,     // 6 valve holding the painting
    REL_7_PIN      // 7 Exit door lock
};

enum REL_INIT {
    REL_0_INIT = 1,     // DESCRIPTION OF THE RELAY WIRING
    REL_1_INIT = 1,     // DESCRIPTION OF THE RELAY WIRING
    REL_2_INIT = 1,     // DESCRIPTION OF THE RELAY WIRING
    REL_3_INIT = 1,     // DESCRIPTION OF THE RELAY WIRING
    REL_4_INIT = 1,     // DESCRIPTION OF THE RELAY WIRING
    REL_DOOR_INIT = 0,  // NC = Empty | COM = 24V  | NO = Valve
    REL_6_INIT = 1,     // NC = Empty | COM = 24V  | NO = Valve
    REL_7_INIT = 1      // DESCRIPTION OF THE RELAY WIRING
};

// relay BASICS
#define REL_AMOUNT 1
const enum REL_PIN relayPinArray[] = {REL_DOOR_PIN};
const enum REL_INIT relayInitArray[] = {REL_DOOR_INIT};

// #define OLED_DISABLE 1
