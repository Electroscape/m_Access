#include "header.h"
#include "header_st.h"
#include "header_s.h"
using namespace stb_namespace;
#include <Keypad.h> /* Standardbibliothek                                                 */
#include <Keypad_I2C.h>
#include "PCF8574.h"
#include <stb_common.h>
#include <stb_brain.h>
#include <avr/wdt.h>
#include <PCF8574.h> /* https://github.com/skywodd/pcf8574_arduino_library - modifiziert!  */

#include <Password.h>
#include <stb_oled.h>
#include <stb_rfid.h>

#define buzzer 3


STB STB;
STB_BRAIN BRAIN;

#ifndef rfidDisable
    // only use software SPI not hardware SPI
    Adafruit_PN532 RFID_0(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_1_SS_PIN);
    Adafruit_PN532 RFID_READERS[1] = {RFID_0};
    uint8_t data[16];
    unsigned long lastRfidCheck = millis();
#endif

/*==KEYPAD I2C============================================================*/
const byte KEYPAD_ROWS = 4;  // Zeilen
const byte KEYPAD_COLS = 3;  // Spalten
const byte KEYPAD_CODE_LENGTH = 4;
const byte KEYPAD_CODE_LENGTH_MAX = 7;

char KeypadKeys[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};


byte KeypadRowPins[KEYPAD_ROWS] = {1, 6, 5, 3};  // Zeilen  - Messleitungen
byte KeypadColPins[KEYPAD_COLS] = {2, 0, 4};     // Spalten - Steuerleitungen (abwechselnd HIGH)

static unsigned long update_timer = millis();
const int keypad_reset_interval = 3000;

Keypad_I2C Keypad(makeKeymap(KeypadKeys), KeypadRowPins, KeypadColPins, KEYPAD_ROWS, KEYPAD_COLS, KEYPAD_ADD, PCF8574_MODE);

// Passwort
Password passKeypad = Password(secret_password);

/*==PCF8574============================================================*/
PCF8574 relay;


bool relay_init() {
    Serial.println("initializing relay");
    relay.begin(RELAY_I2C_ADD);

    for (int i = 0; i < REL_AMOUNT; i++) {
        relay.pinMode(relayPinArray[i], OUTPUT);
        relay.digitalWrite(relayPinArray[i], relayInitArray[i]);
    }

    Serial.print(F("\n successfully initialized relay\n"));
    return true;
}

bool keypad_init() {
    Keypad.addEventListener(keypadEvent);  // Event Listener erstellen
    Keypad.begin(makeKeymap(KeypadKeys));
    Keypad.setHoldTime(5000);
    Keypad.setDebounceTime(20);
    return true;
}

void keypadEvent(KeypadEvent eKey) {
    KeyState state = IDLE;

    state = Keypad.getState();

    switch (state) {
        case PRESSED:
            update_timer = millis();
            switch (eKey) {
                case '#':
                    STB.dbgln("change pass");
                    break;

                case '*':
                    STB.dbgln("reset pass");
                    break;

                default:
                    passKeypad.append(eKey);
                   // printWithHeader(passKeypad.guess, relayCode);
                    break;
            }
            break;

        default:
            break;
    }
}

void setup() {
    
    // starts serial and default oled
    
    STB.begin();
   
    STB.i2cScanner();
    
    Serial.println("WDT endabled");
    wdt_enable(WDTO_8S);
    
    #ifndef rfidDisable
    STB_RFID::RFIDInit(RFID_0);
    #endif
    
    STB.rs485SetSlaveAddr(0);
  
    //BRAIN.receiveFlags(STB);
    

   // if (BRAIN.flags[ledFlag]) {
        // led init
    //}
    pinMode(buzzer,OUTPUT);

    STB.dbgln("Relay: ...");
    if (relay_init()) {
        STB.dbgln("ok");
    }
    wdt_reset();

    //delay(50);

    STB.dbgln("Keypad: ...");
    if (keypad_init()) {
        STB.dbgln(" ok\n");
    }
    wdt_reset();
    delay(5);

    
    
    STB.printSetupEnd();
    
}
    /**
     * @brief Initialize both the buzzer and the rfid to read a card
     * 
     */

void loop() {

    wdt_reset();
    Keypad.getKey();
   // #ifndef rfidDisable
   //     rfidRead();
   // #endif
   // delay(1000);
//
   // tone(buzzer,1700);
   // STB.dbgln("peep for 300ms");
   // delay(300);
   // noTone(buzzer);
   // STB.dbgln("stop for 200ms");
   // delay(200);
    
    //STB.rs485AddToBuffer("my response/data");
    // sends out buffer once polled along with !eof, to handle cmds be aware this sends out the !EOF
    // STB.rs485SlaveRespond();
}

void rfidRead() {
    if (millis() - lastRfidCheck < rfidCheckInterval) {
        return;
    }

    lastRfidCheck = millis();
    char message[32] = "!RFID";

    Serial.println("RFID start");
    Serial.flush();

    for (int readerNo = 0; readerNo < RFID_AMOUNT; readerNo++) {
        if (STB_RFID::cardRead(RFID_READERS[0], data, RFID_DATABLOCK)) {
            Serial.println("RFID read succees");
            Serial.flush();
            strcat(message, "_");
            strcat(message, (char*) data);
        }
    }

    Serial.println("RFID message adding");
    Serial.flush();

    STB.defaultOled.clear();
    STB.defaultOled.println(message);
    STB.rs485AddToBuffer(message);

    Serial.println("RFID end");
    Serial.flush();
}


