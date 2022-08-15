#pragma once

const unsigned long rfidCheckInterval = 250;

#define RFID_AMOUNT         1

const uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

#define RFID_DATABLOCK      1

// --- LED settings --- 
#define NR_OF_LEDS             31  /* Anzahl der Pixel auf einem Strang (Test 1 Pixel)                   */
#define STRIPE_CNT             1

#define RFID_1_LED_PIN          9     /* Per Konvention ist dies RFID-Port 1                                */
#define RFID_2_LED_PIN          6     /* Per Konvention ist dies RFID-Port 2                                */
#define RFID_3_LED_PIN          5     /* Per Konvention ist dies RFID-Port 3                                */
#define RFID_4_LED_PIN          3     /* Per Konvention ist dies RFID-Port 4   */

// 140cm of 60 leds/m = 84 Leds to be safe bump it to 100
int ledCnts[STRIPE_CNT] = {100};
int ledPins[STRIPE_CNT] = {RFID_1_LED_PIN};
