/*
 * Wecker.h
 *
 *  Created on: 17.03.2022
 *      Author: pschneider
 */

#pragma once

#define MAX_WECKER    3

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

enum class TAGE_BIN: uint16_t {
    SO = 0x0001,
    MO = 0x0002,
    DI = 0x0004,
    MI = 0x0008,
    DO = 0x0010,
    FR = 0x0020,
    SA = 0x0040,
    ALL = 0x007F,
    AT = 0x003E,
    WE = 0x0041 
};

struct stWeckZeit {
    WOCHEN_TAG  Wochentag;          
    uint16_t    u16Stunde;
    uint16_t    u16Minute;   
};

class clWecken {
    public:
        ~clWecken(){}
        clWecken (tm *_AktuelleZeit, clOut *_Summer, clIn *_Taster);

        String getWeckStunde(void);
        String getWeckMinute(void); 
        String getTimeString(void);

        bool getStatus(void);
        bool stelleWeckzeit(void);
       
        void setTime(stWeckZeit *_WeckZeit);
        void Start(void); 
        void Stop(void);

        static bool WeckzeitAkivieren(clIn *_Taster);
        static void Check(void);

    private:
        clOut *Summer;
        clIn *Taster;

        static clWecken *pclWecken[MAX_WECKER];
        static uint16_t sWeckerNr;
        
        stWeckZeit WeckZeit;
        
        TAGE_BIN   Wecktage;
        
        struct tm *AktuelleZeit;  

        uint32_t u32Delay;
        
        uint16_t u16Status;
        uint16_t u16StatusWeckzeit;
        uint16_t u16StatusWzAnzeige;
        uint16_t u16InkZeit;
        uint16_t u16InkTage;
        uint16_t u16Count; 
        uint16_t u16WeckerNr;

        uint32_t u32Timer1;
        uint32_t u32Timer2;

        bool bRun;
        bool bTagOk;
        bool bAktive;
        bool bStelleStunden;
        bool bStelleMinuten;
        bool bStelleTage;
        const char *_WeekDay[10] = {"So ", "Mo ", "Di ", "Mi ", "Do ", "Fr ", "Sa ", "So - Sa", "Mo - Fr", "Sa, So"};
                     
        bool inkZeit (uint16_t *u16Zeit, uint16_t u16Grenze);
};