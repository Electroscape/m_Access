#pragma once

const unsigned long rfidCheckInterval = 250;
#define RFID_AMOUNT         1
const uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
#define RFID_DATABLOCK      1


#define KEYPAD_ADD 0x38
char secret_password[] = "5314";

// using PWM 1 (the left most)
#define BUZZER_PIN 9
