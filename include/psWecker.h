/*
 * Wecker.h
 *
 *  Created on: 17.03.2022
 *      Author: pschneider
 */

#pragma once

#define MAX_WECKER    7

enum class WEEK_DAY: uint16_t {
    SO = 0,
    MO = 1,
    DI = 2,
    MI = 3,
    DO = 4,
    FR = 5,
    SA = 6,
    ALL = 7,        // alarm active at all day
    WD = 8,         // alarm active only at work days (MO - FR)
    WE = 9          // alarm active anly at weekend (SA und SO)
};

struct stAlarmTime {
    WEEK_DAY  WeekDay;          
    uint16_t  u16Hour;
    uint16_t  u16Minute;  
    bool      bActive; 
};

class clAlarm {
    public:
        ~clAlarm(){}
        clAlarm ();

        void init(tm *_ActualTime, clOut *_buzzer, clIn *_switch, stAlarmTime *_stWz);

        String getAlarmHour(void);
        String getAlarmMinute(void); 
        String getTimeString(void);
        String getAlarmDay(void);

        uint16_t getAlarmHourValue(void);
        uint16_t getAlarmMinuteValue(void);
        uint16_t getAlarmWeekDayValue(void);

        void setNewAlarmHour(String); 
        void setNewAlarmMinute(String); 
        void setNewWeekDay(String);

        bool getStatus(void);
        bool setNewAlarmTime(void);
        bool setStartStopAlarm(void);
               
        void setTime(stAlarmTime *_AlarmTime);
        void Start(void); 
        void Stop(void);

        static bool enableAlarmTime(clIn *_switch);
        static void Check(void);
  
    private:
        clOut *buzzer = nullptr;
        clIn *cSwitch = nullptr;

        static clAlarm *pclAlarm[MAX_WECKER];
        static uint16_t sAlarmNumber;
                        
        stAlarmTime AlarmTime;
        
        struct tm *ActualTime = nullptr;  

        uint32_t u32Delay;
        
        uint16_t u16AlarmNumber;
        uint16_t u16Status;
        uint16_t u16StatusAlarmTime;
        uint16_t u16StatusWzAnzeige;
        uint16_t u16StatusAlarm;
        uint16_t u16StatusAlarmOld;
        uint16_t u16InkZeit;
        uint16_t u16InkTage;
        uint16_t u16Count; 
        
        uint32_t u32Timer1;
        uint32_t u32Timer2;

        bool bRun;
        bool bTagOk;
        bool bAktive;
        bool bSetAlarmHour;
        bool bSetAlarmMinutes;
        bool bSetAlarmDay;
        const char *_WeekDay[10] = {
            "So         ", 
            "Mo         ", 
            "Di         ", 
            "Mi         ", 
            "Do         ", 
            "Fr         ", 
            "Sa         ", 
            "So - Sa ", 
            "Mo - Fr ", 
            "Sa, So  "
        };
                     
        bool inkZeit (uint16_t *u16Zeit, uint16_t u16Grenze);
};