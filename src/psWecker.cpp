/*
 * Wecker.cpp
 *
 *  Created on: 29.03.2022
 *      Author: pschneider
 */

#include "Arduino.h"
#include "TFT22-Uhr.h"

uint16_t clAlarm::sAlarmNumber = 0;
clAlarm *clAlarm::pclAlarm[MAX_WECKER];

clAlarm::clAlarm (tm *_AktuelleZeit, clOut *_buzzer, clIn *_switch, stAlarmTime *_stWz) {
    u16AlarmNumber = sAlarmNumber++;        // count instances

    if (u16AlarmNumber < MAX_WECKER) {      // save pointer to instance for static functions
        pclAlarm[u16AlarmNumber] = this;
    } 

    buzzer = _buzzer;
    cSwitch = _switch;
    AktuelleZeit = _AktuelleZeit;
                
    bAktive = false;
    bTagOk = false;
    bRun = false;
    bSetAlarmHour = false;
    bSetAlarmMinutes = false;
    bSetAlarmDay = false;

    u16Status = 0;
    u16StatusAlarmTime = 0;
    u16StatusWzAnzeige = 0;
    u16StatusAlarm = 0;
    u16StatusAlarmOld = 10;
    u16InkZeit = 0;
    u16InkTage = 0;
    u16Count = 0; 

    setTime(_stWz);
}

void clAlarm::setTime(stAlarmTime *_AlarmTime) {
    AlarmTime.u16Minute = _AlarmTime->u16Minute;
    AlarmTime.u16Stunde = _AlarmTime->u16Stunde;
    AlarmTime.Wochentag = _AlarmTime->Wochentag;
    bAktive = _AlarmTime->bActive;
}

void clAlarm::setNewAlarmHour(String strData) {
    AlarmTime.u16Stunde = strData.toInt();
}

void clAlarm::setNewAlarmMinute(String strData) {
    AlarmTime.u16Minute = strData.toInt();  
}

void clAlarm::setNewWeekDay(String strData) {
    AlarmTime.Wochentag = (WEEK_DAY)strData.toInt();
}

uint16_t clAlarm::getWeckStundeValue(void) {
    return AlarmTime.u16Stunde;
}

uint16_t clAlarm::getWeckMinuteValue(void) {
    return AlarmTime.u16Minute;
}

uint16_t clAlarm::getWeckWeekDayValue(void) {
    return (uint16_t)AlarmTime.Wochentag;
}

String clAlarm::getWeckStunde(void) {
    return String(AlarmTime.u16Stunde);
}

String clAlarm::getWeckMinute(void) {
    return String(AlarmTime.u16Minute);
}

String clAlarm::getWeckTage(void) {
    return String((uint16_t)AlarmTime.Wochentag);
}

String clAlarm::getTimeString(void) {
    char cStr[40];
   
    if (bAktive) {
        snprintf_P(cStr, sizeof(cStr), PSTR("W%u: * %02u:%02u : %s"), u16AlarmNumber, AlarmTime.u16Stunde, AlarmTime.u16Minute, _WeekDay[(uint16_t)AlarmTime.Wochentag]);
    } else {
        snprintf_P(cStr, sizeof(cStr), PSTR("W%u:   %02u:%02u : %s"), u16AlarmNumber, AlarmTime.u16Stunde, AlarmTime.u16Minute, _WeekDay[(uint16_t)AlarmTime.Wochentag]);
    }

    switch (u16StatusWzAnzeige) { 
        case 0:     // Zeitanzeige komplett
            if (bSetAlarmHour) {
                u32Timer1 = millis();
                u16StatusWzAnzeige = 10;
            } else if (bSetAlarmMinutes) {
                u32Timer1 = millis();
                u16StatusWzAnzeige = 30;
            } else if (bSetAlarmDay) {
                u32Timer1 = millis();
                u16StatusWzAnzeige = 50;
            }
            break;
        case 10:    // hour off 
            if (bAktive) {
                snprintf_P(cStr, sizeof(cStr), PSTR("W%u: *   :%02u : %s"), u16AlarmNumber, AlarmTime.u16Minute, _WeekDay[(uint16_t)AlarmTime.Wochentag]);
            } else {
                snprintf_P(cStr, sizeof(cStr), PSTR("W%u:     :%02u : %s"), u16AlarmNumber, AlarmTime.u16Minute, _WeekDay[(uint16_t)AlarmTime.Wochentag]);
            }
            if (millis() > (u32Timer1 + 300)) {
                u32Timer1 = millis();
                u16StatusWzAnzeige = 20;
            }
            break;
        case 20:    // show complete time string
            if (millis() > (u32Timer1 + 300)) {
                u16StatusWzAnzeige = 0;
            }
            break;
        case 30:    // minutes off
            if (bAktive) {
                snprintf_P(cStr, sizeof(cStr), PSTR("W%u: * %02u:   : %s"), u16AlarmNumber, AlarmTime.u16Stunde, _WeekDay[(uint16_t)AlarmTime.Wochentag]);
            } else {
                snprintf_P(cStr, sizeof(cStr), PSTR("W%u:   %02u:   : %s"), u16AlarmNumber, AlarmTime.u16Stunde, _WeekDay[(uint16_t)AlarmTime.Wochentag]);
            }
            if (millis() > (u32Timer1 + 300)) {
                u32Timer1 = millis();
                u16StatusWzAnzeige = 40;
            }
            break;
        case 40:    // show complete time string
            if (millis() > (u32Timer1 + 300)) {
                u16StatusWzAnzeige = 0;
            }
            break;
        case 50:    // week day off
            if (bAktive) {
                snprintf_P(cStr, sizeof(cStr), PSTR("W%u: * %02u:%02u :   "), u16AlarmNumber, AlarmTime.u16Stunde, AlarmTime.u16Minute);
            } else {
                snprintf_P(cStr, sizeof(cStr), PSTR("W%u:   %02u:%02u :   "), u16AlarmNumber, AlarmTime.u16Stunde, AlarmTime.u16Minute);
            }
            if (millis() > (u32Timer1 + 300)) {
                u32Timer1 = millis();
                u16StatusWzAnzeige = 60;
            }
            break;
        case 60:
            if (millis() > (u32Timer1 + 300)) {
                u16StatusWzAnzeige = 0;
            }
            break;
        default:
            u16StatusWzAnzeige = 0;
            break;
    }

    return String(cStr);
}

bool clAlarm::setNewAlarmTime(void) {
    bool bResult = false;

    switch (u16StatusAlarmTime) {
        case 0:     // wait for switch
            bSetAlarmHour = false;
            bSetAlarmMinutes = false;
            bSetAlarmDay = false;
            if (cSwitch->Status()) {
                bSetAlarmHour = true;
                u16StatusAlarmTime = 10;
            }
            break;
        case 10:    // start function
            if (!cSwitch->Status()) {
                u16StatusAlarmTime = 20;
            }   
            break; 
        case 20:
            if (inkZeit(&AlarmTime.u16Stunde, 24)) {
                bSetAlarmHour = false;
                bSetAlarmMinutes = true;
                u16StatusAlarmTime = 25;
            }
            break;
        case 25:
            if (inkZeit(&AlarmTime.u16Minute, 60)) {
                bSetAlarmHour = false;
                bSetAlarmMinutes = false;
                bSetAlarmDay = true;
                u16StatusAlarmTime = 30;
            }
            break;
        case 30:
            if (inkZeit((uint16_t*)&AlarmTime.Wochentag, 10)) {
                bSetAlarmDay = false;
                bResult = true;
                u16StatusAlarmTime = 0;
            }
            break;
        default:
            bRun = 0;
            u16StatusAlarmTime = 0;
            break;
    }
    return bResult;
}

void clAlarm::Start(void) {
    AlarmTime.bActive = true;
    bAktive = true;     
}

void clAlarm::Stop(void) {
    AlarmTime.bActive = false;
    bAktive = false;     
}

bool clAlarm::getStatus(void) {
    return bAktive;
}

bool clAlarm::inkZeit (uint16_t *_u16Zeit, uint16_t _u16Grenze) {
    bool bResult = false;

    switch (u16InkZeit) {
        case 0:
            u32Timer2 = millis();
            u16InkZeit = 5;
            break;
        case 5:
            if (cSwitch->Status()) {
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
            if (!cSwitch->Status()) {
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

bool clAlarm::setStartStopAlarm(void) {
    bool bResult = false;

    if (u16StatusAlarm != u16StatusAlarmOld) {
		Serial.println(TraceTime() + String("clAlarm::setStartStopAlarm ") + String(u16AlarmNumber) + " - " + String(u16StatusAlarm));
		u16StatusAlarmOld = u16StatusAlarm;
	}

    switch (u16StatusAlarm) {
        case 0:     // wait for switch
            if (!cSwitch->Status()) {
                u16StatusAlarm = 10;
            }
            break;
        case 10:
            if (cSwitch->Status()) {
                bAktive = !bAktive;
                u16StatusAlarm = 20;
            }
            break;
        case 20:    // start function
            if (!cSwitch->Status()) {
                bResult = true;
                u16StatusAlarm = 10;
            }   
            break; 
        default:
            bResult = true;
            u16StatusAlarm = 0;
            break;
    }
    return bResult;
}

// ---------------------------------------------------------------------------------------------------
// static
// ---------------------------------------------------------------------------------------------------
                                                                                                                                                                                                                         
// ---------------------------------------------------------------------------------------------------
// activate alarm time by pushing a switch
// to start push switch for a long time
// ---------------------------------------------------------------------------------------------------
bool clAlarm::enableAlarmTime(clIn *_switch) {
   	static uint16_t u16Status = 0;
	static uint16_t u16Count = 0;
	static uint32_t u32AktiveZeit = 0;

	bool bResult = false;

	switch(u16Status) {
		case 0:                 // wait for function start
			bResult = true;
			if (_switch->StatusLong()) {
				u16Status = 10;
			}
			break;
		case 10:
            if (_switch->StatusLong()) {
                if (pclAlarm[0]->bAktive && pclAlarm[1]->bAktive) {
                    u16Count = 3;
                } else if (pclAlarm[0]->bAktive) {
                    u16Count = 1;
                } else if (pclAlarm[1]->bAktive) {
                    u16Count = 2;
                } else {
                    u16Count = 0;
                }
                u32AktiveZeit = millis();
                u16Status = 20;
            } else {
				u16Status = 0;
			}
			break;
		case 20:
			if (_switch->Status()) {
				switch(u16Count) {
					case 0: 
						pclAlarm[0]->Start(); 
						pclAlarm[1]->Stop();  
						u16Count++;   
						break;
					case 1: 
						pclAlarm[0]->Stop();  
						pclAlarm[1]->Start(); 
						u16Count++;   
						break;
					case 2:	
						pclAlarm[0]->Start(); 
						pclAlarm[1]->Start(); 
						u16Count++;   
						break;
					case 3:	
						pclAlarm[0]->Stop();  
						pclAlarm[1]->Stop();  
						u16Count = 0; 
						break;
					default: 
						u16Count = 0; 
						break;				
				}
				u16Status = 30;
			}

			if (millis() > (u32AktiveZeit + 2000)) {
				saveWeckerConfig();
				u16Status = 0;
			}
			break;
		case 30:
			if (!_switch->Status()) {
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

// ---------------------------------------------------------------------------------------------------
// check alarm time
// ---------------------------------------------------------------------------------------------------
void clAlarm::Check(void) {
    uint16_t u16Count = 0;
     
    for (u16Count = 0; u16Count < sAlarmNumber; u16Count++) {
        if (u16Count >= MAX_WECKER) {
            break;
        }

        pclAlarm[u16Count]->bTagOk = false;

        // check alarm time is active
        if (pclAlarm[u16Count]->AlarmTime.Wochentag == (WEEK_DAY)pclAlarm[u16Count]->AktuelleZeit->tm_wday) {
            pclAlarm[u16Count]->bTagOk = true;        
        } else if (pclAlarm[u16Count]->AlarmTime.Wochentag == WEEK_DAY::ALL) {
            pclAlarm[u16Count]->bTagOk = true;
        } else if (pclAlarm[u16Count]->AlarmTime.Wochentag == WEEK_DAY::WD) {
            if ((pclAlarm[u16Count]->AktuelleZeit->tm_wday > (uint16_t)WEEK_DAY::SO) && 
                (pclAlarm[u16Count]->AktuelleZeit->tm_wday < (uint16_t)WEEK_DAY::SA)) {
                    pclAlarm[u16Count]->bTagOk = true;
            }
        } else if (pclAlarm[u16Count]->AlarmTime.Wochentag == WEEK_DAY::WE) {
            if ((pclAlarm[u16Count]->AktuelleZeit->tm_wday == (uint16_t)WEEK_DAY::SO) ||
                (pclAlarm[u16Count]->AktuelleZeit->tm_wday == (uint16_t)WEEK_DAY::SA)) {
                    pclAlarm[u16Count]->bTagOk = true;
            }
        }
       
        // switch off alarm sound
        if (pclAlarm[u16Count]->cSwitch->Status()) {
            pclAlarm[u16Count]->u16Status = 30;
        }

        switch (pclAlarm[u16Count]->u16Status) {
            case 0:
                if (pclAlarm[u16Count]->bTagOk && pclAlarm[u16Count]->bAktive) {
                    pclAlarm[u16Count]->u16Status = 5; 
                }
                break;
            case 5:
                if (!pclAlarm[u16Count]->bTagOk || !pclAlarm[u16Count]->bAktive) {
                    pclAlarm[u16Count]->u16Status = 0;     
                } else if ((pclAlarm[u16Count]->AlarmTime.u16Minute == pclAlarm[u16Count]->AktuelleZeit->tm_min) && 
                           (pclAlarm[u16Count]->AlarmTime.u16Stunde == pclAlarm[u16Count]->AktuelleZeit->tm_hour)) {
                    pclAlarm[u16Count]->u16Count = 0;
                    pclAlarm[u16Count]->u32Delay = millis();
                    pclAlarm[u16Count]->buzzer->On();
                    pclAlarm[u16Count]->u16Status = 10;
                }
                break;
            case 10:
                if (millis() > (pclAlarm[u16Count]->u32Delay + 500)) {
                    pclAlarm[u16Count]->u32Delay = millis();
                    pclAlarm[u16Count]->buzzer->Off();
                    pclAlarm[u16Count]->u16Status = 20;
                }
                break;
            case 20:
                if (pclAlarm[u16Count]->u16Count >= 300) {
                    pclAlarm[u16Count]->u16Status = 30;
                } else if (millis() > (pclAlarm[u16Count]->u32Delay + 500)) {
                    pclAlarm[u16Count]->u16Count++;
                    pclAlarm[u16Count]->u32Delay = millis();
                    pclAlarm[u16Count]->buzzer->On();
                    pclAlarm[u16Count]->u16Status = 10;
                }
                break;
            case 30:
                pclAlarm[u16Count]->buzzer->Off();
                if (pclAlarm[u16Count]->AlarmTime.u16Minute != pclAlarm[u16Count]->AktuelleZeit->tm_min) {
                    pclAlarm[u16Count]->u16Status = 0;
                }
                break;   
            default:
                pclAlarm[u16Count]->buzzer->Off();
                pclAlarm[u16Count]->u16Status = 0;
                break;
        }
    }
}

uint16_t clAlarm::getNextAlarm(void) {
    uint16_t u16Count = 0;
    uint16_t u16ActHour = pclAlarm[u16Count]->AktuelleZeit->tm_hour;
    uint16_t u16ActMinutes = pclAlarm[u16Count]->AktuelleZeit->tm_min;
    uint16_t u16ActTime = (u16ActHour * 60) + u16ActMinutes;

    uint16_t u16AlarmHour = 0;
    uint16_t u16AlarmMinutes = 0;
    uint16_t u16AlarmTime = 0;

    uint16_t u16TimeDiv = 1000;
    uint16_t u16TimeDivActual = 0;

    uint16_t u16Result = 0;
    static uint16_t u16ResultOld = 0;

    for (u16Count = 0; u16Count < sAlarmNumber; u16Count++) {
        if (u16Count >= MAX_WECKER) {
            break;
        }

        u16AlarmHour = pclAlarm[u16Count]->AlarmTime.u16Stunde;
        u16AlarmMinutes = pclAlarm[u16Count]->AlarmTime.u16Minute;
        if (u16AlarmHour < u16ActHour) {u16AlarmHour = u16AlarmHour + 24;}    // alarm set for the next day
        u16AlarmTime = (u16AlarmHour * 60) + u16AlarmMinutes;     

        u16TimeDivActual = u16AlarmTime - u16ActTime;
     
        if (pclAlarm[u16Count]->AlarmTime.bActive) {            // the alarm must be active to show
            if (u16TimeDiv > u16TimeDivActual) {
                u16Result = u16Count;
                u16TimeDiv = u16TimeDivActual;
            } 
        }
    }

    if (u16ResultOld != u16Result) {
        Serial.println();
        Serial.println("** Nächster Alarm ist W" + String(u16Result));
        u16ResultOld = u16Result;
    }

    return u16Result;
}
