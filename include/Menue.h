/*
 * Menue.h
 *
 *  Created on: 21.03.2022
 *      Author: pschneider
 */

#pragma once

typedef struct {
    bool (*_cb)(void);
    String _Name;
    bool bLastItem;
} menue_t;

class clMenue {
    public:
        ~clMenue(){}
        clMenue (clIn *_Taster, void (*_cbAnzeige)(String)) {
            Taster = _Taster;
            u16Status = 0;
            u16MenueCount = 0;
            cbAnzeige = _cbAnzeige;
        }

        void Verwaltung(menue_t *MenueEintrag) {
            bFertig = MenueEintrag[u16MenueCount]._cb();

            switch (u16Status) {
                case 0:     // init
                    u16MenueCount = 0;
                    cbAnzeige(MenueEintrag[u16MenueCount]._Name);
                    if (!Taster->Status()) {
                        u16Status = 10;
                    }
                    break;
                case 10:    // Warten bis Taster gedrückt wird
                    if (Taster->Status()) {
                        u32Timer = millis();
                        u16Status = 20;
                    }
                    break;
                case 20:    // Zum Starten muss der Taster 3s lang gedrückt werden
                    if (millis() > (u32Timer + 2000)) {
                        u16Status = 30;
                    } else if (!Taster->Status()) {
                        u16Status = 10;
                    }
                    break;
                case 30:    // Menü um 1 weiterschalten
                    if (bFertig) {
                        if (MenueEintrag[u16MenueCount].bLastItem) {
                            u16MenueCount = 0;
                        } else {
                            u16MenueCount++;
                        }
                        cbAnzeige(MenueEintrag[u16MenueCount]._Name);   
                        u16Status = 40;
                    }
                    break;
                case 40:    // Warten bis Tatster nicht mehr gedrückt wird
                    if (!Taster->Status()) {
                        u32Timer = millis();
                        u16Status = 50;
                    }
                    break;
                case 50:    // Warte auf den nächste Tastendruck oder Funktion abrechen
                    if (Taster->Status()) { 
                        u16Status = 30;
                    } else if (millis() > (u32Timer + 3000)) {
                        u16Status = 60;
                    }        
                    break;
                case 60:
                    if (bFertig) {
                        u16Status = 0;
                    }
                    break;
                default:
                    u16Status = 0;
                    break;
            }
        } 

    private:
        clIn *Taster;
        uint16_t u16Status;
        uint16_t u16MenueCount;
        void (*cbAnzeige)(String);	
        uint32_t u32Timer;
        bool bFertig;

};        