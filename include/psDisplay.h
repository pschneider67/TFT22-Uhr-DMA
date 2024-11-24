/*
 * pTft->h
 *
 *  Created on: 24.03.2023
 *      Author: pschneider
 */

// ---------------------------------------------------------------------------------------------------
// Display Höhe = 40 + 100 + 20 + 40 = 200 < 240
// ---------------------------------------------------------------------------------------------------
// 0,0
// _________________  X-Achse width
// |
// |
// |
// | Y-Achse height 
//
// ---------------------------------------------------------------------------------------------------
// Display Aufbau
// Position x Mitte = (DISP_WIDTH / 2) 
// -------------------------------------		oberer Rand y = 0 bis y = 320 Pixel
// Datum und Uhrzeit
// ------------------------------------- 		Y_TOPEnde = 40		
// -------------------------------------    	Anfang mittlerer Bereich Y_MIDDLEStart = 40 + Abstand (8)
//
//   Uhrzeit Anzeige groß
//
// -------------------------------------		Ende mittlerer Bereich Y_MIDDLEEnd = yBottumStart - Abstand (8)
// -------------------------------------		yBottumStart = DISP_WIDTH - 40
// Wlan ID und IP Adresse 
// -------------------------------------		unterer Rand y = DISP_WIDTH, x = tftHeight = 240 Pixel

#pragma once

#include <TFT_eSPI.h> 
#include <string.h>
#include "font.h"

// weather icons 64x64
#include "icons/bild_01d.h"
#include "icons/bild_01n.h"
#include "icons/bild_02d.h"
#include "icons/bild_02n.h"
#include "icons/bild_03d.h"
#include "icons/bild_04d.h"
#include "icons/bild_09d.h"
#include "icons/bild_10d.h"
#include "icons/bild_10n.h"
#include "icons/bild_11d.h"
#include "icons/bild_13d.h"
#include "icons/bild_50d.h"
#include "icons/bild_44.h"

class clDisplay : public TFT_eSPI {
  public:
    clDisplay();
    ~clDisplay(){}
  
    void showVersion(void);
    void showWeather(const char* _strIcon, uint16_t _xpos, uint16_t _ypos);
    void showWeatherIcon(const unsigned short* _image, uint16_t _xpos, uint16_t _ypos);
    void showState(const char*); 
    void showFrame(void);
    void clearMiddleArea(void);
    void clearDisplay(void);
    void showDateAndTime(struct tm _actTimeinfo, const char *_WeekDay);
    void fillSpriteBlack(TFT_eSprite*);
    void showTime(struct tm _actTimeinfo, bool _bForce);
    
    uint16_t getXPosWeatherNow(void){return X_POS_WEATHER_NOW;};
    uint16_t getYPosWeatherNow(void){return Y_POS_WEATHER_NOW;};
    uint16_t getYMiddle(void) {return Y_MIDDLE;};
    uint16_t getSpriteWidth(void) {return spriteWidth;};
    uint16_t getSpriteXPos(void) {return spriteXPos;};
    uint16_t getSpriteYPos(void) {return spriteYPos;};
    uint16_t getHSpace(void) {return H_SPACE;};

    void initTimeToShow();

    const GFXfont *DefaultFont = &Arimo10pt7b;
    const GFXfont *TimeFont = &Arimo_Bold_Time54pt7b;

    TFT_eSprite* actTimeToShow = NULL; 
	  TFT_eSprite* actDateToShow = NULL;
	  TFT_eSprite* actTimeSecToShow = NULL;  

  private:
    const uint16_t DISP_HIGTH = 240; 
    const uint16_t DISP_WIDTH = 320;

    const uint16_t H_SPACE    = 8;			  // space
    const uint16_t Y_TOP      = 0;			  // start upper area
    const uint16_t H_TOP      = 35;		    // higth of upper area

    const uint16_t H_BOTTOM   = 35;        
    const uint16_t Y_BOTTOM   = (DISP_HIGTH - H_BOTTOM);    // start lower area

    const uint16_t Y_MIDDLE   = (H_TOP + H_SPACE);   	      // start middle area
    const uint16_t H_MIDDLE   = ((DISP_HIGTH - H_TOP - H_BOTTOM) - (2 * H_SPACE));  // higth of middle area

    const uint16_t X_POS_WEATHER_NOW = (2 * H_SPACE);
    const uint16_t Y_POS_WEATHER_NOW = (Y_BOTTOM - (H_SPACE + 2) - 64);

    uint16_t spriteWidth = DISP_WIDTH - (4 * H_SPACE);
	  uint16_t spriteHigth = 81;
	  uint16_t spriteYPos  = Y_MIDDLE + (H_SPACE / 2);
	  uint16_t spriteXPos  = DISP_WIDTH - spriteWidth - (2 * H_SPACE);	

    uint16_t u16StateShowTime = 0;
    tm tmTimeOld;
	 
    String strTimeOld = " ";
    String strDateOld = " ";

    char strShowStateOld[40] = {"                                       "};
};