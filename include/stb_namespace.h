#pragma once

#include <Arduino.h>
#include <PCF8574.h> /* I2C Port Expander  */
#include <Wire.h>    /* I2C library */

// Defined by hardware
#define RELAY_I2C_ADD 0x3F
#define MAX_CTRL_PIN A0
#define MAX485_WRITE HIGH
#define MAX485_READ LOW

namespace stb_namespace {
    class Brain {
       private:
        /* data */
       public:
        Brain(String BrainName);
        ~Brain();
    };
    // static unsigned long lastHeartbeat = millis();
    // static unsigned long heartbeatFrequency = 3000;

    //void printWithHeader(String message, String source=String("SYS"));
    // void heartbeat();
    void softwareReset();
    bool i2cScanner();
    bool brainSerialInit();
}  // namespace stb_namespace
