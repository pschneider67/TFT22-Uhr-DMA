/*
 * Wecker.cpp
 *
 *  Created on: 29.03.2022
 *      Author: pschneider
 */

#include "Arduino.h"
#include "TFT22-Uhr.h"

uint16_t clWecken::sWeckerNr = 0;
clWecken *clWecken::pclWecken[MAX_WECKER];

clWecken::clWecken (struct tm *_AktuelleZeit, clOut *_Summer, clIn *_Taster) {
    u16WeckerNr = sWeckerNr++;          // zähle die Instanzen

    if (u16WeckerNr < MAX_WECKER) {     // speichere einen Pointer auf diese Instanz
        pclWecken[u16WeckerNr] = this;
    }

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

void clWecken::setTime(stWeckZeit *_WeckZeit) {
    WeckZeit.u16Minute = _WeckZeit->u16Minute;
    WeckZeit.u16Stunde = _WeckZeit->u16Stunde;
    WeckZeit.Wochentag = _WeckZeit->Wochentag;
}

String clWecken::getWeckStunde(void) {
    return String(WeckZeit.u16Stunde);
}

String clWecken::getWeckMinute(void) {
    return String(WeckZeit.u16Minute);
}

String clWecken::getTimeString(void) {
    char cStr[25];
    String Ausgabe;

    if (bAktive) {
        sprintf(cStr, "W%u: * %02u:%02u : ", u16WeckerNr + 1, WeckZeit.u16Stunde, WeckZeit.u16Minute);
    } else {
        sprintf(cStr, "W%u:   %02u:%02u : ", u16WeckerNr + 1, WeckZeit.u16Stunde, WeckZeit.u16Minute);
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
                sprintf(cStr, "W%u: *   :%02u : ", u16WeckerNr + 1, WeckZeit.u16Minute);
            } else {
                sprintf(cStr, "W%u:     :%02u : ", u16WeckerNr + 1, WeckZeit.u16Minute);
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
                sprintf(cStr, "W%u: * %02u:   : ", u16WeckerNr + 1, WeckZeit.u16Stunde);
            } else {
                sprintf(cStr, "W%u:   %02u:   : ", u16WeckerNr + 1, WeckZeit.u16Stunde);
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

bool clWecken::stelleWeckzeit(stWeckZeit *_Weckzeit) {
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

void clWecken::Start(void) {
    bAktive = true;     
}

void clWecken::Stop(void) {
    bAktive = false;     
}

bool clWecken::getStatus(void) {
    return bAktive;
}

void clWecken::Check(void) {
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

bool clWecken::inkZeit (uint16_t *_u16Zeit, uint16_t _u16Grenze) {
    bool bResult = false;

    switch (u16InkZeit) {
        case 0:
            if (Taster->Status()) {
                if (++*_u16Zeit == _u16Grenze) {
                    *_u16Zeit = 0;
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
            if (millis() > (u32Timer2 + 150)) {
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

// ---------------------------------------------------------------------------------------------------
// statische Methode
// ---------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------
// Aktiviere Weckzeiten mit Taster 
// Zum Starten muss die Taste 3sec lang gedückt werden
// ---------------------------------------------------------------------------------------------------
// Tastendruck 		Wecker 1		Wecker 2
//       1             1               0
//       2             0               1
//       3             1               1
//       4             0               0
// ---------------------------------------------------------------------------------------------------
bool clWecken::WeckzeitAkivieren(clIn *_Taster) {
   	static uint16_t u16Status = 0;
	static uint16_t u16StateOld = 1;
	static uint16_t u16Count = 0;
	static uint32_t u32AktuelleZeit = 0;
	static uint32_t u32AktiveZeit = 0;

	bool bResult = false;

	if (u16Status != u16StateOld)
	{
		Serial.println(TraceTime() + "WA Status - " + String(u16Status));
		u16StateOld = u16Status;
	}

	switch(u16Status) {
		case 0:
			bResult = true;
			if (_Taster->Status()) {
				u32AktuelleZeit = millis();
				u16Status = 10;
			}
			break;
		case 10:
			if ((millis() - u32AktuelleZeit) >= 3000) {
				if (_Taster->Status()) {
					if (pclWecken[0]->getStatus() && pclWecken[1]->getStatus()) {
						u16Count = 3;
					} else if (pclWecken[0]->getStatus()) {
						u16Count = 1;
					} else if (pclWecken[1]->getStatus()) {
						u16Count = 2;
					} else {
						u16Count = 0;
					}
					u32AktiveZeit = millis();
					u16Status = 20;
				}
			} else if (!_Taster->Status()) {
				u16Status = 0;
			}
			break;
		case 20:
			if (_Taster->Status()) {
				switch(u16Count) {
					case 0: 
						pclWecken[0]->Start(); 
						pclWecken[1]->Stop();  
						u16Count++;   
						break;
					case 1: 
						pclWecken[0]->Stop();  
						pclWecken[1]->Start(); 
						u16Count++;   
						break;
					case 2:	
						pclWecken[0]->Start(); 
						pclWecken[1]->Start(); 
						u16Count++;   
						break;
					case 3:	
						pclWecken[0]->Stop();  
						pclWecken[1]->Stop();  
						u16Count = 0; 
						break;
					default: 
						u16Count = 0; 
						break;				
				}
				u32AktuelleZeit = millis();
				u16Status = 30;
			}

			if (millis() > (u32AktiveZeit + 2000)) {
				saveWeckerConfig();
				u16Status = 0;
			}
			break;
		case 30:
			if (!_Taster->Status()) {
				u32AktiveZeit = millis();
				u16Status = 20;
			}
			break;
		default:
			u16Status = 0;
			break;
	}
	return bResult;
}