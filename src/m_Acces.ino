#include "header_st.h"                                     
#include <Keypad_I2C.h>
#include <stb_brain.h>
#include <avr/wdt.h>
#include <PCF8574.h> /* https://github.com/skywodd/pcf8574_arduino_library - modifiziert!  */

#include <Password.h>
#include <stb_rfid.h>
#include <stb_keypadCmds.h>
#include <stb_oledCmds.h>


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
 - Dynamischer Headline text wie "Enter Code", "Welcome" etc over cmd from Mother
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
 - rs485Write with option to not erase buffer or mby a flag in STB
*/


STB_BRAIN Brain;


// only use software SPI not hardware SPI
Adafruit_PN532 RFID_0(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_1_SS_PIN);
Adafruit_PN532 RFID_READERS[1] = {RFID_0};
uint8_t data[16];
unsigned long lastRfidCheck = millis();

Keypad_I2C Keypad(makeKeymap(KeypadKeys), KeypadRowPins, KeypadColPins, KEYPAD_ROWS, KEYPAD_COLS, KEYPAD_ADD, PCF8574_MODE);

// the Evaluation is done on the Mother, may be kept for convenience or removed later
Password passKeypad = Password(dummyPassword);
unsigned long lastKeypadAction = millis();

// freq is unsinged int, duration is unsigned long
// ticks remaning, frequency, duration
int buzzerStatus[3] = {1, 1000, 50};


void setup() {
    
    // starts serial and default oled
    
    Brain.begin();
    Brain.STB_.dbgln("WDT endabled");
    wdt_enable(WDTO_8S);

    Brain.setSlaveAddr(0);

    // Brain.receiveFlags();
    // for ease of developmen
    Brain.flags[rfidFlag] = 1;
    Brain.flags[keypadFlag] = 1;


    buzzer_init();
    if (Brain.flags[rfidFlag]) {
        STB_RFID::RFIDInit(RFID_0);
    }

    if (Brain.flags[keypadFlag]) {
        keypad_init();
    }
 
    Brain.STB_.printSetupEnd();

    // TODO: replace this, going to be done from Mother
    STB_OLED::writeHeadline(&Brain.STB_.defaultOled, "ENTER CODE");
}


void loop() {

    if (Brain.flags[keypadFlag]) {
        Keypad.getKey();
    }
    
    if (Brain.flags[rfidFlag]) {
        rfidRead();
    }
    
    keypadResetCheck();
    buzzerUpdate();

    Brain.slaveRespond();

    interpreter();

    wdt_reset();
}


void interpreter() {
    /*
        - check headline
        - check correct/wrong
        - toggle RFID/Keypad
        - needs to handle multiple lines
    */

    while (Brain.STB_.rcvdPtr != NULL) {
        checkForValid();


        Brain.nextRcvdLn();
    }
}


// checks what the oled headline should display
bool checkForHeadline() {
    if ( strncmp(oledHeaderCmd.c_str(), Brain.STB_.rcvdPtr, oledHeaderCmd.length()) == 0) {
        char *cmdPtr = strtok(Brain.STB_.rcvdPtr, KeywordsList::delimiter.c_str());
        cmdPtr = strtok(NULL, KeywordsList::delimiter.c_str());
        STB_OLED::writeHeadline(&Brain.STB_.defaultOled, cmdPtr);
        return true;
    }
    return false;
}


// checks keypad feedback, its only correct/incorrect
bool checkForValid() {
    if (strncmp(keypadCmd.c_str(), Brain.STB_.rcvdPtr, keypadCmd.length()) == 0) {
        // do i need a fresh char pts here?
        char *cmdPtr = strtok(Brain.STB_.rcvdPtr, KeywordsList::delimiter.c_str());
        cmdPtr = strtok(NULL, KeywordsList::delimiter.c_str());
        int cmdNo;
        sscanf(cmdPtr, "%d", &cmdNo);
        if (cmdNo == KeypadCmds::correct) {
            // beep correct
        } else {
            // beep false
        }
        return true;
    }
    return false;
}

// checks if RFID or Keypad should be toggled
bool toggled() {
    return false;
}

// --- Keypad


void keypad_init() {
    Brain.STB_.dbgln(F("Keypad: ..."));
    Keypad.addEventListener(keypadEvent);  // Event Listener erstellen
    Keypad.begin(makeKeymap(KeypadKeys));
    Keypad.setHoldTime(5000);
    Keypad.setDebounceTime(20);
    Brain.STB_.dbgln(F(" ok\n"));
}


void keypadEvent(KeypadEvent eKey) {
    KeyState state = IDLE;

    state = Keypad.getState();

    Brain.STB_.dbgln(String(eKey));

    if (state == PRESSED) {
        lastKeypadAction = millis();
    }

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
                    // currently optional and nice to have but other things are prio
                    // Brain.STB_.rs485AddToBuffer(passKeypad.guess);
                    // TODO: probably going to modify this
                    STB_OLED::writeCenteredLine(&Brain.STB_.defaultOled, passKeypad.guess);
                    tone(BUZZER_PIN, 1700, 100);
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
    char msg[20]; 
    strcpy(msg, keypadCmd.c_str());
    strcat(msg, KeywordsList::delimiter.c_str());
    char noString[3];
    sprintf(noString, "%i", KeypadCmds::evaluate);
    strcat(msg, noString);
    strcat(msg, KeywordsList::delimiter.c_str());
    strcat(msg, passKeypad.guess);
    Brain.addToBuffer(msg, true);
}


void passwordReset() {
    if (strlen(passKeypad.guess) > 0) {
        passKeypad.reset();
        // Todo: consider updating the mother but it may be just more practical to respond on the Poll 
    }
}

void keypadResetCheck() {
    if (millis() - lastKeypadAction > keypadResetInterval) {
        checkPassword();
    }
}


// --- Buzzer


void buzzer_init() {
    pinMode(BUZZER_PIN,OUTPUT);
    noTone(BUZZER_PIN);
}

void buzzerUpdate() {
    if (buzzerStatus[0] > 0) {
        tone(BUZZER_PIN, buzzerStatus[1], (unsigned long) buzzerStatus[2]);
        buzzerStatus[0]--;
    }
}


void doubleBeep(){
    // TODO: do this with buzzerStatus
    tone(BUZZER_PIN,1700);
    delay(300);
    noTone(BUZZER_PIN);
    delay(200);
    tone(BUZZER_PIN,1700);
    delay(800);
    noTone(BUZZER_PIN);
    Serial.println("doubleBeep");
}


// --- RFID


void rfidRead() {
    
    if (millis() - lastRfidCheck < rfidCheckInterval) {
        return;
    }

    lastRfidCheck = millis();
    char message[16];

    for (int readerNo = 0; readerNo < RFID_AMOUNT; readerNo++) {
        if (STB_RFID::cardRead(RFID_READERS[0], data)) {
            strcpy(message, KeywordsList::rfidKeyword.c_str());
            // strcat(message, "_");
            strcat(message, (char*) data);
            doubleBeep(); // maybe replace this here with setting buzzerStatus
            Brain.STB_.rs485AddToBuffer(message);
        }
    }

}


