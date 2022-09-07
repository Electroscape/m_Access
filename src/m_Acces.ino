#include "header_st.h"                                     
#include <Keypad_I2C.h>
#include <stb_brain.h>
#include <avr/wdt.h>
#include <PCF8574.h> /* https://github.com/skywodd/pcf8574_arduino_library - modifiziert!  */

#include <Password.h>
#include <stb_oled.h>
#include <stb_rfid.h>


/*
TODO:
 - (optional) make buzzer fncs non blocking to not interfere with the bus unnecessarily
 - ensure critical messages are confirmed like evaluation, may be best to do this in lib arduino with a flag for the slaverespond
 - periodic updates on the password? everytime its polled? reset being handled by mother or brain?
 - get defenition when what beeping must occur
 - make buzzer non blocking, tone already supports a single non blocking beep, doubles probably need fnc
*/

/*
Fragen and access module Requirements
 - RS485 optimierung welche prio? 
 - relay requirements? Wann brauchen wir das relay-breakout?
 - Dynamischer Headline text wie "Enter Code", "Welcome" etc
 - Necessity toggle between RFID and Keypad
 Änderung hier wird erstmal über THT gemacht aber zwischenzeitlich kann das über SMD gemacht werden, was aber auch erstmal bestellt werden muss
 - Welches feedback kommt beim RFID? Beinhaltet das feedback das Oled?
 - No specific code on brain?
 - when does what beeping must occur
Es soll der jeweilige Status per RS485 an die Mother geschickt werden: 
Code korrekt, Karte korrekt. Mehr ist aktuell nicht erforderlich
https://www.meistertask.com/app/task/mZyBg1ER/access-modul-1-keypad-modul-mit-rfid

 - lauflicht funktionalität in library 
 - mehrfache LED werte übergen in einem packet
 - Check relays limits aktuell
*/


STB_BRAIN Brain;


// only use software SPI not hardware SPI
Adafruit_PN532 RFID_0(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_1_SS_PIN);
Adafruit_PN532 RFID_READERS[1] = {RFID_0};
uint8_t data[16];
unsigned long lastRfidCheck = millis();


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

Keypad_I2C Keypad(makeKeymap(KeypadKeys), KeypadRowPins, KeypadColPins, KEYPAD_ROWS, KEYPAD_COLS, KEYPAD_ADD, PCF8574_MODE);

// Passwort
Password passKeypad = Password(secret_password);


void setup() {
    
    // starts serial and default oled
    
    Brain.begin();
    Brain.STB_.dbgln("WDT endabled");
    wdt_enable(WDTO_8S);

    Brain.setSlaveAddr(0);

    // Brain.receiveFlags();


    buzzer_init();
    if (Brain.flags[rfidFlag]) {
        STB_RFID::RFIDInit(RFID_0);
    }

    if (Brain.flags[keypadFlag]) {
        keypad_init();
    }
 
    Brain.STB_.printSetupEnd();

    STB_OLED::writeHeadline(&Brain.STB_.defaultOled, "ENTER CODE");
}


void loop() {

    if (Brain.flags[keypadFlag]) {
        Keypad.getKey();
    }
    
    if (Brain.flags[rfidFlag]) {
        rfidRead();
    }
    Brain.slaveRespond();

    wdt_reset();
}


// --- Keypad


void keypad_init() {
    Brain.STB_.dbgln("Keypad: ...");
    Keypad.addEventListener(keypadEvent);  // Event Listener erstellen
    Keypad.begin(makeKeymap(KeypadKeys));
    Keypad.setHoldTime(5000);
    Keypad.setDebounceTime(20);
    Brain.STB_.dbgln(" ok\n");
}


void keypadEvent(KeypadEvent eKey) {
    KeyState state = IDLE;

    state = Keypad.getState();

    switch (state) {
        case PRESSED:
            switch (eKey) {
                case '#':
                    checkPassword();
                    doubleBeep();
                    break;

                case '*':
                    passwordReset();
                    break;

                default:
                    passKeypad.append(eKey);
                    Brain.STB_.rs485AddToBuffer(passKeypad.guess);
                    STB_OLED::writeCenteredLine(&Brain.STB_.defaultOled, passKeypad.guess);
                    beep500();
                    break;
            }
            break;

        default:
            break;
    }
}


/**
 * @brief checks the password and displays
 * @return true 
 * @return false 
 */
void checkPassword() {
    if (strlen(passKeypad.guess) < 1) return;


    // TBD evaluation on the Mother iirc

    /*
    bool result = passKeypad.evaluate();

    if (result) {
        // display some stuff
    } else {

    }

    return result;
    */
}


void passwordReset() {
    if (strlen(passKeypad.guess) > 0) {
        passKeypad.reset();
    }
}


// --- Buzzer


void buzzer_init(){
    pinMode(BUZZER_PIN,OUTPUT);
}


void beep500(){
    tone(BUZZER_PIN,1700);
    delay(100);
    noTone(BUZZER_PIN);
    Brain.STB_.dbgln("peep for 500ms");
}


void doubleBeep(){
    tone(BUZZER_PIN,1700);
    delay(300);
    noTone(BUZZER_PIN);
    delay(200);
    tone(BUZZER_PIN,1700);
    delay(800);
    noTone(BUZZER_PIN);
    Brain.STB_.dbgln("peep sequence");
}


// --- RFID


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
            doubleBeep();
        }
    }

    Serial.println("RFID message adding");
    Serial.flush();

    
    //STB.defaultOled.println(message);
    Brain.STB_.defaultOled.clear();
    //STB.defaultOled.println(message);
    Brain.STB_.rs485AddToBuffer(message);
    // STB.defaultOled.println(STB.bufferOut);

    Serial.println("RFID end");
    Serial.flush();
}


