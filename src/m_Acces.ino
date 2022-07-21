#include "header.h"

#include <stb_common.h>
#include <stb_brain.h>

STB STB;
STB_BRAIN BRAIN;


void setup() {
    
    STB.begin();
    STB.i2cScanner();
    
    STB.rs485SetSlaveAddr(0);
  
    BRAIN.receiveFlags(STB);

    if (BRAIN.flags[ledFlag]) {
        // led init
    }
    
    STB.printSetupEnd();
}


void loop() {
    STB.rs485SlaveRespond();
}
