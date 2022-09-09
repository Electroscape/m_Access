#include "header_st.h"

#include <stb_mother.h>
#include <stb_keypadCmds.h>

STB_MOTHER Mother;
int stage = 0;


void setup() {

    Mother.begin();
    // starts serial and default oled

    Serial.println("WDT endabled");
    wdt_enable(WDTO_8S);

    Mother.rs485SetSlaveCount(1);

    // Mother.setFlag(STB, 0, cmdFlags::LED, true);

    // STB.printSetupEnd();
}


bool checkForKeypad() {
    if (passwordMap[stage] < 0) { return false; }

    if (strncmp(keypadCmdKeyword.c_str(), Mother.STB_.rcvdPtr, keypadCmdKeyword.length()) == 0) {
        
        char *cmdPtr = strtok(Mother.STB_.rcvdPtr, KeywordsList::delimiter.c_str());
        cmdPtr = strtok(NULL, KeywordsList::delimiter.c_str());
        int cmdNo;
        sscanf(cmdPtr, "%d", &cmdNo);

        if (cmdNo == KeypadCmds::evaluate) {

            cmdPtr = strtok(NULL, KeywordsList::delimiter.c_str());
            // TODO: error handling here in case the rest of the msg is lost?
            if (!(cmdPtr != NULL)) {
                // send NACK? this isnt in the control flow yet
                return false;
            }

            Mother.sendAck();
            if (strncmp(passwords[stage], cmdPtr, strlen(passwords[stage]) ) == 0) {
                char msg[10] = "";
                strcpy(msg, keypadCmdKeyword.c_str());
                // slaveNo needs to be really optional at this point. the default shall be polledslave
                Mother.sendCmdToSlave(Mother.rs485getPolledSlave(), msg);
            }
        }
        // TODO: check for ACK handling on Brain
        
        return true;
    }
    return false;
}


bool checkForRfid() {
    if (passwordMap[stage] < 0) { return false; }
    return false;
}


void interpreter() {
    while (Mother.STB_.rcvdPtr != NULL) {
        if (checkForKeypad()) {continue;};
        if (checkForRfid()) {continue;};
        Mother.nextRcvdLn();
    }
}


void loop() {
    Mother.rs485PerformPoll();
    interpreter();
    // Mother.sendCmdToSlave(1, msg);
}




