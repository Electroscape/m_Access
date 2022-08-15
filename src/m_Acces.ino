#include "header.h"
#include "header_st.h"
#include <stb_common.h>
#include <stb_brain.h>
#include <avr/wdt.h>
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

void setup() {
    
    // starts serial and default oled
    
    STB.begin();
    STB.serialInit();
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
    
    
    STB.printSetupEnd();
    
}
    /**
     * @brief Initialize both the buzzer and the rfid to read a card
     * 
     */

void loop() {

    wdt_reset();
    #ifndef rfidDisable
        rfidRead();
    #endif
    delay(1000);

    tone(buzzer,1700);
    STB.dbgln("peep for 300ms");
    delay(300);
    noTone(buzzer);
    STB.dbgln("stop for 200ms");
    delay(200);
    
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


