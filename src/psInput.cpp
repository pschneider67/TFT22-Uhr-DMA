/*
 * psInput.cpp
 *
 *  Created on: 04.04.2022
 *      Author: pschneider
 */

#include "Arduino.h"
#include "TFT22-Uhr.h"

uint16_t clIn::u16InCountMax = 0;

clIn::clIn() {
    u16InCountMax++;
    u16InCount = u16InCountMax;
    
    u16Status = 0;
    u16StatusOld = 1;

    bStatus = false;
    bShort = false;
    bLong = false;
}

void clIn::Init(stInput _Param) {
    Param = _Param;

    pinMode(Param.pin, INPUT);

    if (Param.irq) {
        noInterrupts();   // disable irq 
        if (Param.polarity == POLARITY::POS) {
            attachInterrupt(digitalPinToInterrupt(Param.pin), Param.cb, Param.mode);
        } else {
            attachInterrupt(digitalPinToInterrupt(Param.pin), Param.cb, Param.mode);
        }
        Serial.println("** install irq for input " + (String)Param.pin);
        interrupts();     // enable irq
    }
}

void clIn::runState(void) {

    if (u16Status != u16StatusOld) {
        //Serial.println(String("Input " + String(u16InCount) + String(" Status - " + String(u16Status))));
        u16StatusOld = u16Status;
    }

    SetStatus();

    switch (u16Status) {
        case 0:
            bShort = false;
            bLong = false;
            if (bStatus) {
                u32Timer = millis();
                u16Status = 10;
            }
            break;
        case 10:
            if (!bStatus) {
                Serial.println(F("** switch active"));
                u16Status = 0;
            } else if (millis() > (u32Timer + Param.entprellzeit)) {
                bShort = true;
                u32Timer = millis();
                u16Status = 20;	
            }					
            break;
        case 20:
            if (!bStatus) {
                u16Status = 0;	
            } else if (millis() > (u32Timer + Param.switchLongTime)) {
                bLong = true;
                u16Status = 30;
            }
            break;
        case 30:
            if (!bStatus) {
                u16Status = 0;	
            }
            break;
        default:
            u16Status = 0;
            break;
    }
}

void clIn::SetStatus(void) {
    if (Param.polarity == POLARITY::POS) {
        bStatus = digitalRead(Param.pin);
    } else {
        bStatus = ~digitalRead(Param.pin);
    }
}
