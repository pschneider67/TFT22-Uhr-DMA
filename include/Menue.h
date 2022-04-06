/*
 * Menue.h
 *
 *  Created on: 21.03.2022
 *      Author: pschneider
 */

#pragma once
typedef struct {
    bool (*_cbMenue)(void);     // Menüfunktion
    String _MenueName;          // Menüname
    bool _bLastItem;            // Ende der Liste
} menue_t;

class clMenue {
    public:
        ~clMenue(){}
        clMenue (clIn *_MenueTaster, menue_t *_MenueArray, void (*_cbAnzeige)(String)) {
            MenueTaster = _MenueTaster;
            MenueArray = _MenueArray;
            cbAnzeige = _cbAnzeige;
            u16MenueCount = 0;
        }

        void Verwaltung(void) {
            static uint16_t u16Status = 0;

            // Menüfunktion aufrufen
            // Wird "true" zurückgegeben, dann wurde die Funktion komplett abgearbeitet
            // oder noch nicht getartet
            bMenueFertig = MenueArray[u16MenueCount]._cbMenue();

            switch (u16Status) {
                case 0:     // init
                    if (!MenueTaster->Status()) { 
                        u16MenueCount = 0;                                  // aktuell aktives Menü
                        cbAnzeige(MenueArray[u16MenueCount]._MenueName);    // aktuellen Menüname anzeigen
                        u16Status = 10;
                    }
                    break;
                case 10:    // Zum Starten muss der Taster 2s lang gedrückt werden
                    if (MenueTaster->StatusLong()) {
                        u16Status = 20;
                    } 
                    break;
                case 20:    // Menü um 1 Menüpunkt weiterschalten
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
                case 30:    // Warten bis Tatster nicht mehr gedrückt wird
                    if (!MenueTaster->Status()) {
                        u32Timer = millis();
                        u16Status = 40;
                    }
                    break;
                case 40:    // Warte auf den nächste Tastendruck oder Funktion abrechen
                    if (MenueTaster->Status()) { 
                        u16Status = 20;
                    } else if (millis() > (u32Timer + 3000)) {
                        u16Status = 50;
                    } 
                    break;
                case 50:
                    if (bMenueFertig) {
                        u16Status = 0;
                    }
                    break;
                default:
                    u16Status = 0;
                    break;
            }
        } 

    private:
        void (*cbAnzeige)(String);	
        clIn *MenueTaster;
        uint16_t u16MenueCount;
        uint32_t u32Timer;
        menue_t *MenueArray;
        bool bMenueFertig;
    };        