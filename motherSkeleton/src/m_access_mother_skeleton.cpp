#include "header_st.h"

#include <stb_mother.h>

STB_MOTHER Mother;


void setup() {

    Mother.begin();
    // starts serial and default oled

    Serial.println("WDT endabled");
    wdt_enable(WDTO_8S);

    Mother.rs485SetSlaveCount(1);

    // Mother.setFlag(STB, 0, cmdFlags::LED, true);

    // STB.printSetupEnd();
}


void loop() {
    Mother.rs485PerformPoll();
    char msg[] = "response to brain ";
    Mother.sendCmdToSlave(1, msg);
}
