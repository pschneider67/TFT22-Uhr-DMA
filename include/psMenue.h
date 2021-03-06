/*
 * Menue.h
 *
 *  Created on: 21.03.2022
 *      Author: pschneider
 */

#pragma once
typedef struct {
    bool (*_cbMenue)(void);     // menue function
    String _MenueName;          // menue name
    bool _bLastItem;            // end of menu list
} menue_t;

class clMenue {
    public:
        ~clMenue(){}
        clMenue (clIn *_switch, menue_t *_MenueArray, void (*_cbAnzeige)(String));
        bool runMenue(void);
        uint16_t getAktualMenue(void);

    private:
        void (*cbAnzeige)(String);	
        clIn *Sw;
        uint16_t u16MenueCount;
        uint32_t u32Timer;
        menue_t *MenueArray;
        bool bFuncReady;

        uint16_t u16RunStatus;
        uint16_t u16RunStatusOld;
    };        