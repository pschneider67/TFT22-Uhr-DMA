/*
 * Menue.cpp
 *
 *  Created on: 29.03.2022
 *      Author: pschneider
 */

#include "Arduino.h"
#include "TFT22-Uhr.h"

clMenue::clMenue(clIn *_MenueTaster, menue_t *_MenueArray, void (*_cbAnzeige)(String)) {
    MenueTaster = _MenueTaster;
    MenueArray = _MenueArray;
    cbAnzeige = _cbAnzeige;
    u16MenueCount = 0;
}

uint16_t clMenue::getAktualMenue(void) {
    return u16MenueCount;
}

bool clMenue::Verwaltung(void) {
    bool bResult = false;
    static uint16_t u16Status = 0;

    // call menue function
    // return "true" --> function is ready or not started yet
    bMenueFertig = MenueArray[u16MenueCount]._cbMenue();

    switch (u16Status) {
        case 0:     // init
            if (!MenueTaster->Status()) { 
                u16MenueCount = 0;                                  // actual menue number
                cbAnzeige(MenueArray[u16MenueCount]._MenueName);    // show actual menue
                u16Status = 10;
            }
            break;
        case 10:    // to start, push switch  for 2s
            if (MenueTaster->StatusLong()) {
                u16Status = 20;
            } 
            break;
        case 20:    // set next meune from list
            if (bMenueFertig) {
                if (MenueArray[u16MenueCount]._bLastItem) {
                    u16MenueCount = 0;
                } else {
                    u16MenueCount++;
                }
                cbAnzeige(MenueArray[u16MenueCount]._MenueName);    // neuen Menüpunkt anzeigen
                u16Status = 30;
            } 
            break;
        case 30:    // wait for switch off
            if (!MenueTaster->Status()) {
                u32Timer = millis();
                u16Status = 40;
            }
            break;
        case 40:    // wait for switch on aor timeout
            if (MenueTaster->Status()) { 
                u16Status = 20;
            } else if (millis() > (u32Timer + 3000)) {
                u16Status = 50;
            } 
            break;
        case 50:
            if (bMenueFertig) {
                bResult = true;
                u16Status = 0;
            }
            break;
        default:
            u16Status = 0;
            break;
    }
    return bResult;
} 