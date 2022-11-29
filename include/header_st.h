#pragma once

const unsigned long rfidCheckInterval = 250;
#define RFID_AMOUNT         1

// using PWM 1 (the left most)
#define BUZZER_PIN 9
#define BuzzerMaxStages 3

/*==KEYPAD I2C============================================================*/
#define KEYPAD_ADD 0x38

char dummyPassword[] = "";
const char processingText[] PROGMEM = {"..."};

const byte KEYPAD_ROWS = 4;  // Zeilen
const byte KEYPAD_COLS = 3;  // Spalten
const byte KEYPAD_CODE_LENGTH = 4;
const byte KEYPAD_CODE_LENGTH_MAX = 8;

const int keypadResetInterval = 3000;
const unsigned long rfidCooldown = 8000;
const unsigned int keypadBaseTone = 294;
const unsigned int keypadPitchMultiplier = 40;

char KeypadKeys[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};


byte KeypadRowPins[KEYPAD_ROWS] = {1, 6, 5, 3};  // Zeilen  - Messleitungen
byte KeypadColPins[KEYPAD_COLS] = {2, 0, 4};     // Spalten - Steuerleitungen (abwechselnd HIGH)

