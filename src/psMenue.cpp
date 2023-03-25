/*
 * Menue.cpp
 *
 *  Created on: 29.03.2022
 *      Author: pschneider
 */

#include "Arduino.h"
#include "TFT22-Uhr.h"

clMenue::clMenue(clIn *_switch, menue_t *_MenueArray, void (*_cbAnzeige)(const char*)) {
    Sw = _switch;
    MenueArray = _MenueArray;
    cbAnzeige = _cbAnzeige;
    u16MenueCount = 0;

    u16RunStatus = 0;
    u16RunStatusOld = 10;
}

uint16_t clMenue::getAktualMenue(void) {
    return u16MenueCount;
}

bool clMenue::handle(void) {
    bool bResult = false;
    
    if (u16RunStatus != u16RunStatusOld) {
		Serial.println(TraceTime() + String("clMenue::handle - ") + String(u16RunStatus));
		u16RunStatusOld = u16RunStatus;
	}

    // call menue function
    // return "true" --> function is ready or not started yet
    bFuncReady = MenueArray[u16MenueCount]._cbMenue();

    switch (u16RunStatus) {
        case 0:     // init
            if (!Sw->Status()) { 
                u16MenueCount = 0;                                  // actual menue number
                cbAnzeige(MenueArray[u16MenueCount]._pMenueName);   // show actual menue
                u16RunStatus = 10;
            }
            break;
        case 10:    // to start, push switch  for 2s
            if (bFuncReady && (Sw->StatusLong())) {
                u16RunStatus = 20;
            } 
            break;
        case 20:    // set next meune from list
            if (bFuncReady) {
                if (MenueArray[u16MenueCount]._bLastItem) {
                    u16MenueCount = 0;
                } else {
                    u16MenueCount++;
                }
                cbAnzeige(MenueArray[u16MenueCount]._pMenueName);    // show new menu text
                u16RunStatus = 30;
            } else {    // function is running
                u16RunStatus = 50;
            } 
            break;
        case 30:    // wait for switch off
            if (bFuncReady && (!Sw->Status())) {
                u32Timer = millis();
                u16RunStatus = 40;
            } 
            break;
        case 40:    // wait for switch on or timeout
            if (Sw->Status()) { 
                u16RunStatus = 20;
            } else if (millis() > (u32Timer + 3000)) {
                u16RunStatus = 50;
            } 
            break;
        case 50:
            if (bFuncReady) {
                bResult = true;
                u16RunStatus = 0;
            }
            break;
        default:
            u16RunStatus = 0;
            break;
    }
    return bResult;
} 