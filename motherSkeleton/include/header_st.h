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
    "Stage 0",
    "Stage 1",
    "Stage 2",
    "Stage 3",
    "Stage 4",
    "Stage 5",
    "Stage 6",
    "Stage 7",
    "Stage 8",
    "Stage 9"
};

// defines what password/RFIDCode is used at what stage, if none is used its -1
int passwordMap[StageCount] = {
    0,
    1,
    2,
    3,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
};

// save what already is turned on on the brain so we do not need to send it again
int devicesOn = 0;
// how to handle what is active? do a binary addition to run a code?
// so make it 2^Flag?
int devicesToggle[StageCount] = {
    int( pow(2, keypadFlag ) + pow(2, oledFlag)),
    int( pow(2, rfidFlag) + pow(2, oledFlag)) ,
    int( pow(2, keypadFlag ) + pow(2, oledFlag)),
    int( pow(2, keypadFlag ) + pow(2, oledFlag)),
    int( pow(2, keypadFlag ) + pow(2, oledFlag)),
    int( pow(2, keypadFlag ) + pow(2, oledFlag)),
    int( pow(2, keypadFlag ) + pow(2, oledFlag)),
    int( pow(2, keypadFlag ) + pow(2, oledFlag)),
    int( pow(2, keypadFlag ) + pow(2, oledFlag)),
    int( pow(2, keypadFlag ) + pow(2, oledFlag))
};