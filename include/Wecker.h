/*
 * Wecker.h
 *
 *  Created on: 17.03.2022
 *      Author: pschneider
 */

#pragma once

#define MAX_WECKER    2

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
        clWecken (struct tm *_AktuelleZeit, clOut *_Summer, clIn *_Taster);

        String getWeckStunde(void);
        String getWeckMinute(void); 
        String getTimeString(void);

        bool stelleWeckzeit(stWeckZeit *_Weckzeit);
        bool getStatus(void);

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
        bool bTagOk;
        bool bAktive;
        bool bStelleStunden;
        bool bStelleMinuten;
        const char *_WeekDay[10] = {"So ", "Mo ", "Di ", "Mi ", "Do ", "Fr ", "Sa ", "So - Sa", "Mo - Fr", "Sa, So"};
                     
        bool inkZeit (uint16_t *u16Zeit, uint16_t u16Grenze);
};