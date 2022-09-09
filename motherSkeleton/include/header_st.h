#pragma once

#define StageCount 10
#define passCnt 4
#define maxPassLen 10

// may aswell move this into the Oled lib?
#define headLineMaxSize 14

char passwords[passCnt][maxPassLen] = {
    "1234",
    "4321",
    "1337",
    "0000"
};

char stageTexts[StageCount][headLineMaxSize] = {
    "Wecome",
    "Enter Code",
    "Present RFID"
};

// defines what password/RFIDCode is used at what stage, if none is used its -1
int passwordMap[StageCount] = {
    0,
    1,
    2,
    3,
    -1,
    -1,
    -1
};