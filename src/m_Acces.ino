/**
 * @file m_Acces.ino
 * @author Martin Pek (martin.pek@web.de)
 * @brief access module supports RFID and Keypad authentication with oled&buzzer feedback
 * @version 0.1
 * @date 2022-09-09
 * 
 * @copyright Copyright (c) 2022
 * 
 */

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
build for lib_arduino 0.6.7 onwards
TODO:
 - (optional) make buzzer fncs non blocking to not interfere with the bus unnecessarily
 - ensure critical messages are confirmed like evaluation, may be best to do this in lib arduino with a flag for the slaverespond
 - periodic updates on the password? everytime its polled? reset being handled by mother or brain?
 - get defenition when what beeping must occur
 - make buzzer non blocking, tone already supports a single non blocking beep, doubles probably need fnc
*/

/*
🔲✅
Fragen and access module Requirements
 - RS485 optimierung welche prio? 
 - relay requirements? Wann brauchen wir das relay-breakout?
 - ✅ Dynamischer Headline text wie "Enter Code", "Welcome" etc over cmd from Mother
 - ✅ Necessity toggle between RFID and Keypad
 Änderung hier wird erstmal über THT gemacht aber zwischenzeitlich kann das über SMD gemacht werden, was aber auch erstmal bestellt werden muss
 - Welches feedback kommt beim RFID? Beinhaltet das feedback das Oled?

 - lauflicht funktionalität in library 
 - mehrfache LED werte übergen in einem packet
 - Check relays limits aktuell
 - rs485Write with option to not erase buffer or mby a flag in STB
 - split poll into pull and push cmd then handled seperately
 - consider a generic non specifc clearing of oled
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

unsigned long lastOledAction = millis();
// technically only the 3rd line i will update
bool oledHasContent = false;
// freq is unsinged int, durations are unsigned long
// frequency, ontime, offtime
unsigned int buzzerFreq[BuzzerMaxStages] = {0};
unsigned long buzzerOn[BuzzerMaxStages] = {0};
unsigned long buzzerOff[BuzzerMaxStages] = {0};
unsigned long buzzerTimeStamp = millis();

int buzzerStage = -1;


void setup() {
    
    // starts serial and default oled
    
    Brain.begin();
    Brain.STB_.dbgln("v0.09");
    wdt_enable(WDTO_8S);

    Brain.setSlaveAddr(0);

    // Brain.receiveFlags();
    // for ease of developmen
    Brain.flags = rfidFlag + oledFlag + keypadFlag;


    buzzer_init();
    if (Brain.flags & rfidFlag) {
        STB_RFID::RFIDInit(RFID_0);
    }

    if (Brain.flags & keypadFlag) {
        keypad_init();
    }
 
    Brain.STB_.printSetupEnd();

    STB_OLED::writeHeadline(&Brain.STB_.defaultOled, "Booting");
}


void loop() {

    wdt_reset();
    
    if (Brain.flags & keypadFlag) {
        Keypad.getKey();
    }
    
    if (Brain.flags & rfidFlag) {
        rfidRead();
    }
    
    keypadResetCheck();
    buzzerUpdate();
    oledResetCheck();

    if (!Brain.slaveRespond()) {
        return;
    }

    while (Brain.STB_.rcvdPtr != NULL) {
        // Serial.println("Brain.STB_.rcvdPtr");
        interpreter();
        Brain.nextRcvdLn();
    }
}


void interpreter() {
    if (checkForValid()) {return;}
    if (checkForHeadline()) {return;}
    Brain.receiveFlags();
}


// checks what the oled headline should display
bool checkForHeadline() {
    if ( strncmp(oledHeaderCmd.c_str(), Brain.STB_.rcvdPtr, oledHeaderCmd.length()) == 0) {
        Brain.sendAck();
        char *cmdPtr = strtok(Brain.STB_.rcvdPtr, KeywordsList::delimiter.c_str());
        cmdPtr = strtok(NULL, KeywordsList::delimiter.c_str());
        STB_OLED::writeHeadline(&Brain.STB_.defaultOled, cmdPtr);
        return true;
    }
    return false;
}


// --- Buzzer 

/**
 * @brief Set buzzer timings to be excecuter
 * @param i 
 * @param freq 
 * @param ontime 
 * @param offtime 
*/
void setBuzzerStage(int i, unsigned int freq, unsigned long ontime, unsigned long offtime=0) {
    buzzerFreq[i] = freq;
    buzzerOn[i] = ontime;
    buzzerOff[i] = offtime;
    buzzerStage = 0;
}


void buzzer_init() {
    pinMode(BUZZER_PIN,OUTPUT);
    noTone(BUZZER_PIN);
}


// may change this to contain differen lengts for on and off depending on reqs
void buzzerUpdate() {
    if (buzzerStage < 0) {return;}
    if (buzzerStage >= BuzzerMaxStages) {
        buzzerReset();
        return;
    }

    if (buzzerStage == 0 || millis() > buzzerTimeStamp) {
        // moves next execution to after the on + offtime elapsed 
        buzzerTimeStamp = millis() + buzzerOn[buzzerStage] + buzzerOff[buzzerStage];
        if (buzzerFreq[buzzerStage] > 0) {
            tone(BUZZER_PIN, buzzerFreq[buzzerStage], buzzerOn[buzzerStage]);
            buzzerStage++;
            return;
        } else {
            buzzerReset();
        }
    }
}


void buzzerReset() {
    for (int i=0; i<BuzzerMaxStages; i++) {
        buzzerStage = -1;
        buzzerFreq[i] = 0;
    }
}


// ---


// checks keypad feedback, its only correct/incorrect
bool checkForValid() {

    // Serial.print("checking: ");
    // Serial.println(Brain.STB_.rcvdPtr);
    
    if (strncmp(keypadCmd.c_str(), Brain.STB_.rcvdPtr, keypadCmd.length()) == 0) {
        Brain.sendAck();
        // Serial.println("incoming keypadCmd");
        // do i need a fresh char pts here?
        char *cmdPtr = strtok(Brain.STB_.rcvdPtr, KeywordsList::delimiter.c_str());
        cmdPtr = strtok(NULL, KeywordsList::delimiter.c_str());
        int cmdNo;
        sscanf(cmdPtr, "%d", &cmdNo);

        if (cmdNo == KeypadCmds::correct) {
            setBuzzerStage(0, 1000, 400, 200);
            setBuzzerStage(1, 1500, 1500, 0);
            STB_OLED::writeToLine(&Brain.STB_.defaultOled, 3, F("valid"), true);
            // tone(BUZZER_PIN, 1700, 1000);
        } else {
            setBuzzerStage(0, 800, 200, 50);
            setBuzzerStage(1, 400, 400, 100);
            setBuzzerStage(2, 400, 600, 200);
            STB_OLED::writeToLine(&Brain.STB_.defaultOled, 3, F("invalid"), true);
        }
        return true;
    }
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

    if (state == PRESSED) {
        lastKeypadAction = millis();
    }

    switch (state) {
        case PRESSED:
            switch (eKey) {
                case '#':
                    checkPassword();
                    // some beep? or only after mother answers?
                    break;

                case '*':
                    passwordReset();
                    break;

                default:
                    if (oledHasContent) {return;}
                    passKeypad.append(eKey);
                    if (strlen(passKeypad.guess) >= KEYPAD_CODE_LENGTH_MAX) {
                        checkPassword();
                    }
                    // currently optional and nice to have but other things are prio
                    // Brain.STB_.rs485AddToBuffer(passKeypad.guess);
                    // TODO: probably going to modify this this needs to be centered line 2
                    STB_OLED::writeToLine(&Brain.STB_.defaultOled, 2, passKeypad.guess, true);
                    tone(BUZZER_PIN, 1700, 100);
                    break;
            }
            break;

        default:
            break;
    }
}


/**
 * @brief sends the password to the Mother for evaluation
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
    passwordReset();
    STB_OLED::writeToLine(&Brain.STB_.defaultOled, 2, F("..."), true);
    oledHasContent = true;
    lastOledAction = millis();
    Brain.addToBuffer(msg, true);
}


void oledReset() {
    STB_OLED::clearAbove(Brain.STB_.defaultOled, 2);
    oledHasContent = false;
}

void passwordReset() {
    if (strlen(passKeypad.guess) > 0) {
        passKeypad.reset();
        oledReset();
        // oled clear?
        // Todo: consider updating the mother but it may be just more practical to respond on the Poll 
    }
    
    
}


void keypadResetCheck() {
    if (millis() - lastKeypadAction > keypadResetInterval) {
        checkPassword();
    }
}


void oledResetCheck() {
    if (!oledHasContent) {return;}
    if (millis() - lastOledAction > keypadResetInterval) {
        oledReset();
    }
}



// --- RFID


void rfidRead() {
    
    if (millis() - lastRfidCheck < rfidCheckInterval) {
        return;
    }

    lastRfidCheck = millis();
    char message[20];

    for (int readerNo = 0; readerNo < RFID_AMOUNT; readerNo++) {
        if (STB_RFID::cardRead(RFID_READERS[0], data)) {
            strcpy(message, KeywordsList::rfidKeyword.c_str());
            strcat(message, (char*) data);
            // tone(BUZZER_PIN, 1700, 100);
            Brain.addToBuffer(message);
            // simply adding a delay here, alternatively we save the RFID card identy and dont rescan the same
            lastRfidCheck = millis() + 8000;
        }
    }

}


