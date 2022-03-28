/*
 * Wecker.h
 *
 *  Created on: 17.03.2022
 *      Author: pschneider
 */

#pragma once

#include "EEPROM.h"

enum class WOCHEN_TAG: uint16_t {
    SO = 0,
    MO = 1,
    DI = 2,
    MI = 3,
    DO = 4,
    FR = 5,
    SA = 6,
    ALL = 7,        // jeder Tag
    AT = 8,         // Arbeitstage (MO - FR)
    WE = 9          // Wochenende (SA und SO)
};

struct stWeckZeit {
    WOCHEN_TAG  Wochentag;          
    uint16_t    u16Stunde;
    uint16_t    u16Minute;   
};

class clWecken {
    public:
        ~clWecken(){}
        clWecken (struct tm *_AktuelleZeit, clOut *_Summer, clIn *_Taster) {
            static uint16_t _WeckerNr = 0;

            u16WeckerNr = ++_WeckerNr;

            Summer = _Summer;
            Taster = _Taster;
            AktuelleZeit = _AktuelleZeit;
                        
            bAktive = false;
            bRun = false;
            bStelleStunden = false;
            bStelleMinuten = false;

            u16Status = 0;
            u16StatusWeckzeit = 0;
            u16StatusWzAnzeige = 0;
            u16InkZeit = 0;
            u16Count = 0; 
        }

        void setTime(stWeckZeit *_WeckZeit) {
            WeckZeit.u16Minute = _WeckZeit->u16Minute;
            WeckZeit.u16Stunde = _WeckZeit->u16Stunde;
            WeckZeit.Wochentag = _WeckZeit->Wochentag;
        }

        String getWeckStunde(void) {
            return String(WeckZeit.u16Stunde);
        }

        String getWeckMinute(void) {
            return String(WeckZeit.u16Minute);
        }

        bool getStatus(void) {
            return bAktive;
        }

        String getTimeString(void) {
            char cStr[25];
            String Ausgabe;

            if (bAktive) {
                sprintf(cStr, "W%u: * %02u:%02u : ", u16WeckerNr, WeckZeit.u16Stunde, WeckZeit.u16Minute);
            } else {
                sprintf(cStr, "W%u:   %02u:%02u : ", u16WeckerNr, WeckZeit.u16Stunde, WeckZeit.u16Minute);
            }

            switch (u16StatusWzAnzeige) { 
                case 0:     // Zeitanzeige komplett
                    if (bStelleStunden) {
                        u32Timer1 = millis();
                        u16StatusWzAnzeige = 10;
                    } else if (bStelleMinuten) {
                        u32Timer1 = millis();
                        u16StatusWzAnzeige = 30;
                    }
                    break;
                case 10:    // Anzeige Stunden aus
                    if (bAktive) {
                        sprintf(cStr, "W%u: *   :%02u : ", u16WeckerNr, WeckZeit.u16Minute);
                    } else {
                        sprintf(cStr, "W%u:     :%02u : ", u16WeckerNr, WeckZeit.u16Minute);
                    }
                    if (millis() > (u32Timer1 + 300)) {
                        u32Timer1 = millis();
                        u16StatusWzAnzeige = 20;
                    }
                    break;
                case 20:    // Zeitanzeige komplett
                    if (millis() > (u32Timer1 + 300)) {
                        u16StatusWzAnzeige = 0;
                    }
                    break;
                case 30:    // Anzeige Minuten aus
                    if (bAktive) {
                        sprintf(cStr, "W%u: * %02u:   : ", u16WeckerNr, WeckZeit.u16Stunde);
                    } else {
                        sprintf(cStr, "W%u:   %02u:   : ", u16WeckerNr, WeckZeit.u16Stunde);
                    }
                    if (millis() > (u32Timer1 + 300)) {
                        u32Timer1 = millis();
                        u16StatusWzAnzeige = 40;
                    }
                    break;
                case 40:    // Zeitanzeige komplett
                    if (millis() > (u32Timer1 + 300)) {
                        u16StatusWzAnzeige = 0;
                    }
                    break;
                default:
                    u16StatusWzAnzeige = 0;
                    break;

            }
           
            Ausgabe  = String(cStr);
            Ausgabe += String(_WeekDay[(uint16_t)WeckZeit.Wochentag]);

            return Ausgabe;
        }
        
        bool stelleWeckzeit(stWeckZeit *_Weckzeit) {
            bool bResult = false;

            switch (u16StatusWeckzeit) {
                case 0:     // warte auf Testendruck
                    bStelleStunden = false;
                    bStelleMinuten = false;
                    if (Taster->Status()) {
                        u32Timer2 = millis();
                        u16StatusWeckzeit = 10;
                    }
                    break;
                case 10:    // nach 2s Funktion beginnen
                    if (millis() > (u32Timer2 + 2000)) {
                        bStelleStunden = true;    
                        u16StatusWeckzeit = 15;
                    } else if (!Taster->Status()) {
                        u16StatusWeckzeit = 0;
                    }
                    break;              
                case 15:
                    if (!Taster->Status()) {
                        u32Timer2 = millis();
                        u16StatusWeckzeit = 20;
                    }   
                    break; 
                case 20:
                    if (inkZeit(&WeckZeit.u16Stunde, 24)) {
                        bStelleStunden = false;
                        bStelleMinuten = true;
                        u16StatusWeckzeit = 25;
                    }
                    break;
                case 25:
                    if (inkZeit(&WeckZeit.u16Minute, 60)) {
                        bStelleStunden = false;
                        bStelleMinuten = false;
                        bResult = true;
                        u16StatusWeckzeit = 0;
                    }
                    break;
                default:
                    bRun = 0;
                    u16StatusWeckzeit = 0;
                    break;
            }
            return bResult;
        }

        void Start(void) {
            bAktive = true;     
        }

        void Stop(void) {
            bAktive = false;     
        }

        bool GetStatus(void) {
            return bAktive;
        }

        void Check(void) {
            bool bTagOk = false;

            // Prüfe ob Weckzeit aktiv ist    
            if (!bAktive) {
                return;
            }

            // Prüfe ob der Wecker aktiv ist
            if (WeckZeit.Wochentag == (WOCHEN_TAG)AktuelleZeit->tm_wday) {
                bTagOk = true;        
            } else if (WeckZeit.Wochentag == WOCHEN_TAG::ALL) {
                bTagOk = true;
            } else if (WeckZeit.Wochentag == WOCHEN_TAG::AT) {
                if ((AktuelleZeit->tm_wday > (uint16_t)WOCHEN_TAG::SO) && 
                    (AktuelleZeit->tm_wday < (uint16_t)WOCHEN_TAG::SA)) {
                        bTagOk = true;
                }
            } else if (WeckZeit.Wochentag == WOCHEN_TAG::WE) {
                if ((AktuelleZeit->tm_wday == (uint16_t)WOCHEN_TAG::SO) ||
                    (AktuelleZeit->tm_wday == (uint16_t)WOCHEN_TAG::SA)) {
                        bTagOk = true;
                }
            }

            // Abfrage od der richtige Tag für die Weckzeit erreicht ist
            if (!bTagOk) {
                return;
            }

            // Weckenton ausschalten
            if (Taster->Status()) {
                u16Status = 30;
            }

            switch (u16Status) {
                case 0:
                    if ((WeckZeit.u16Minute == AktuelleZeit->tm_min) && (WeckZeit.u16Stunde == AktuelleZeit->tm_hour)) {
                        u16Count = 0;
                        u32Delay = millis();
                        Summer->On();
                        u16Status = 10;
                    }
                    break;
                case 10:
                    if (millis() > (u32Delay + 500)) {
                        u32Delay = millis();
                        Summer->Off();
                        u16Status = 20;
                    }
                    break;
                case 20:
                    if (u16Count >= 100) {
                        u16Status = 30;
                    } else if (millis() > (u32Delay + 500)) {
                        u16Count++;
                        u32Delay = millis();
                        Summer->On();
                        u16Status = 10;
                    }
                    break;
                case 30:
                    Summer->Off();
                    if (WeckZeit.u16Minute != AktuelleZeit->tm_min) {
                        u16Status = 0;
                    }
                    break;   
                default:
                    Summer->Off();
                    u16Status = 0;
                    break;
            }
        }

    private:
        clOut *Summer;
        clIn *Taster;
        
        stWeckZeit WeckZeit;
        
        struct tm *AktuelleZeit;  
        
        uint32_t u32Delay;
        
        uint16_t u16Status;
        uint16_t u16StatusWeckzeit;
        uint16_t u16StatusWzAnzeige;
        uint16_t u16InkZeit;
        uint16_t u16Count; 
        uint16_t u16WeckerNr;

        uint32_t u32Timer1;
        uint32_t u32Timer2;

        bool bRun;
        bool bAktive;
        bool bStelleStunden;
        bool bStelleMinuten;
        const char *_WeekDay[10] = {"So ", "Mo ", "Di ", "Mi ", "Do ", "Fr ", "Sa ", "So - Sa", "Mo - Fr", "Sa, So"};

        bool inkZeit (uint16_t *u16Zeit, uint16_t u16Grenze) {
            bool bResult = false;

            switch (u16InkZeit) {
                case 0:
                    if (Taster->Status()) {
                        if (++*u16Zeit == u16Grenze) {
                            *u16Zeit = 0;
                        }
                        u32Timer2 = millis();
                        if (bRun) {
                            u16InkZeit = 15;
                        } else {
                            u16InkZeit = 10;
                        }
                    } else if (millis() > (u32Timer2 + 2000)) {
                        bRun = false;
                        u32Timer2 = millis();
                        bResult = true;
                        u16InkZeit = 0;
                    }   
                    break;
                case 10:
                    if (!Taster->Status()) {
                        u32Timer2 = millis();
                        u16InkZeit = 0;
                    } else if (millis() > (u32Timer2 + 1000)) {
                        bRun = true;
                        u32Timer2 = millis();
                        u16InkZeit = 0;
                    }
                    break;
                case 15:
                    if (millis() > (u32Timer2 + 200)) {
                        u16InkZeit = 0;
                    }
                    break;
                default:
                    bRun = false;
                    u16InkZeit = 0;
                    break; 
        }
        return bResult;
    }
};