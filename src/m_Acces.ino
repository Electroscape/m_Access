#include "header.h"

#include <stb_common.h>
#include <stb_brain.h>
#include <avr/wdt.h>

STB STB;
STB_BRAIN BRAIN;


void setup() {
    
    // starts serial and default oled
    STB.begin();
    
    Serial.println("WDT endabled");
    wdt_enable(WDTO_8S);
    
    STB.i2cScanner();
    
    STB.rs485SetSlaveAddr(0);
  
    BRAIN.receiveFlags(STB);

    if (BRAIN.flags[ledFlag]) {
        // led init
    }
    
    STB.printSetupEnd();
}


void loop() {
    STB.rs485AddToBuffer("my response/data");
    // sends out buffer once polled along with !eof, to handle cmds be aware this sends out the !EOF
    STB.rs485SlaveRespond();
}
