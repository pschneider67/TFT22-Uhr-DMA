/*
 * tft.cpp
 *
 *  Created on: 24.03.2023
 *      Author: pschneider
 */

#include "psDisplay.h"

extern char cVersion[] PROGMEM;
extern char cDatum[]   PROGMEM;

clDisplay::clDisplay() {
	actTimeToShow    = new TFT_eSprite(this); 	// sprite to show actual time
	actDateToShow    = new TFT_eSprite(this);		// sprite to show actual date
	actTimeSecToShow = new TFT_eSprite(this);		// sprite to show actual time and sec.

	initTimeToShow();

	begin();
}
 
void clDisplay::showVersion(void) {
	char strText[40];
	snprintf_P(strText, sizeof(strText), PSTR("Ver. %s - %s"), cVersion, cDatum); 
	showState(strText);
}

void clDisplay::showWeather(const char* _strIcon, uint16_t _xpos, uint16_t _ypos) {
	if      (strcmp(_strIcon, "01d") == 0) {showWeatherIcon(bild_01d, _xpos, _ypos);}
	else if (strcmp(_strIcon, "01n") == 0) {showWeatherIcon(bild_01n, _xpos, _ypos);}
	else if (strcmp(_strIcon, "02d") == 0) {showWeatherIcon(bild_02d, _xpos, _ypos);}
	else if (strcmp(_strIcon, "02n") == 0) {showWeatherIcon(bild_02n, _xpos, _ypos);}
	else if (strcmp(_strIcon, "03d") == 0) {showWeatherIcon(bild_03d, _xpos, _ypos);}
	else if (strcmp(_strIcon, "03n") == 0) {showWeatherIcon(bild_03d, _xpos, _ypos);}
	else if (strcmp(_strIcon, "04d") == 0) {showWeatherIcon(bild_04d, _xpos, _ypos);}
	else if (strcmp(_strIcon, "04n") == 0) {showWeatherIcon(bild_04d, _xpos, _ypos);}
	else if (strcmp(_strIcon, "09d") == 0) {showWeatherIcon(bild_09d, _xpos, _ypos);}
	else if (strcmp(_strIcon, "09n") == 0) {showWeatherIcon(bild_09d, _xpos, _ypos);}
	else if (strcmp(_strIcon, "10d") == 0) {showWeatherIcon(bild_10d, _xpos, _ypos);}
	else if (strcmp(_strIcon, "10n") == 0) {showWeatherIcon(bild_10n, _xpos, _ypos);}
	else if (strcmp(_strIcon, "13d") == 0) {showWeatherIcon(bild_13d, _xpos, _ypos);}
	else if (strcmp(_strIcon, "13n") == 0) {showWeatherIcon(bild_13d, _xpos, _ypos);}
	else if (strcmp(_strIcon, "50d") == 0) {showWeatherIcon(bild_50d, _xpos, _ypos);}
	else if (strcmp(_strIcon, "50n") == 0) {showWeatherIcon(bild_50d, _xpos, _ypos);}
	else {showWeatherIcon(bild_44, _xpos, _ypos);}
}

void clDisplay::showWeatherIcon(const unsigned short* _image, uint16_t _xpos, uint16_t _ypos) {
	uint16_t u16Width = 64;		// icon size
	uint16_t u16Higth = 64;
	
	fillRect(_xpos, _ypos, u16Width, u16Higth, TFT_BLACK);	
	pushImage(_xpos, _ypos, 64, 64, _image);
}

void clDisplay::showState(const char* _strData) {
	setFreeFont(DefaultFont);
	setTextSize(1);	

	// clear actual string
	setTextColor(TFT_BLACK, TFT_BLACK);
	drawCentreString(strShowStateOld, DISP_WIDTH / 2, Y_BOTTOM + 8, 1);

	// draw new string
	setTextColor(TFT_YELLOW, TFT_BLACK);
	drawCentreString(_strData, DISP_WIDTH / 2, Y_BOTTOM + 8, 1);
	setFreeFont(NULL);

	strcpy(strShowStateOld, _strData);
}

void clDisplay::showFrame(void) {
	fillScreen(TFT_BLACK);
	drawRoundRect(1, Y_TOP, DISP_WIDTH - 1, H_TOP, 10, TFT_BLUE);
	drawRoundRect(1, Y_MIDDLE, DISP_WIDTH - 1, H_MIDDLE, 10, TFT_BLUE);
	drawRoundRect(1, Y_BOTTOM, DISP_WIDTH - 1, H_BOTTOM, 10, TFT_BLUE);
}

void clDisplay::initTimeToShow() {
	String strInfo = "wait for weather data";

	actTimeToShow->setColorDepth(8);
	actTimeToShow->createSprite(spriteWidth, spriteHigth);
	actTimeToShow->setFreeFont(DefaultFont);
	actTimeToShow->setTextSize(1);		
	actTimeToShow->fillSprite(TFT_BLACK);

	actTimeToShow->setTextColor(TFT_YELLOW);
	actTimeToShow->drawCentreString(strInfo, spriteWidth / 2, (spriteHigth / 2), 1);
	actTimeToShow->pushSprite(spriteXPos, spriteYPos);			// show sprite at screen
}

void clDisplay::clearMiddleArea(void) {
	fillRect(5, Y_MIDDLE + 5, DISP_WIDTH - 10, H_MIDDLE - 10, TFT_BLACK);
	setFreeFont(DefaultFont);
	setTextSize(1);	
	setTextColor(TFT_YELLOW, TFT_BLACK);
	setFreeFont(DefaultFont);
}
    
void clDisplay::clearDisplay(void) {
	fillScreen(TFT_BLACK);
	setFreeFont(DefaultFont);
}

void clDisplay::showDateAndTime(struct tm _actTimeinfo, const char *_WeekDay) {
	uint16_t spriteHigth = 18;
	
	// date
	uint16_t spriteWidth1 = (DISP_WIDTH / 2) - 20;
	uint16_t spriteXPos1  = H_SPACE;	
	uint16_t spriteYPos1  = 9;

	// time
	uint16_t spriteWidth2 = (DISP_WIDTH / 2) - 75;
	uint16_t spriteXPos2  = (DISP_WIDTH / 2) + 75 - H_SPACE;	
	uint16_t spriteYPos2  = 9;

	char str[20];

	String strTime;
	String strDate;
		
	snprintf_P(str, sizeof(str), PSTR("%02u:%02u:%02u"), _actTimeinfo.tm_hour, _actTimeinfo.tm_min, _actTimeinfo.tm_sec);
	strTime = String(str);
		
	// write actual time if time changed
	if (strTimeOld != strTime) {
		actTimeSecToShow->setColorDepth(8);
		actTimeSecToShow->createSprite(spriteWidth2, spriteHigth);
		actTimeSecToShow->fillSprite(TFT_BLACK);
		
		actTimeSecToShow->setFreeFont(DefaultFont);
		actTimeSecToShow->setTextSize(1);	
		actTimeSecToShow->setTextColor(TFT_YELLOW);
		actTimeSecToShow->drawRightString(strTime, spriteWidth2, (spriteHigth / 2) - 10, 1);
		
		actTimeSecToShow->pushSprite(spriteXPos2, spriteYPos2);	// show sprite at screen
		strTimeOld == strTime;
	}

	snprintf_P(str, sizeof(str), PSTR("%s"), _WeekDay);
	
	strDate = String(str);
	snprintf_P(str, sizeof(str), PSTR("%02u.%02u.%4u"), _actTimeinfo.tm_mday, _actTimeinfo.tm_mon + 1, 1900 + _actTimeinfo.tm_year);
	strDate += String(str);

	// write actual date if time changed
	if (strDateOld != strDate) {
		actDateToShow->setColorDepth(8);
		actDateToShow->createSprite(spriteWidth1, spriteHigth);
		actDateToShow->fillSprite(TFT_BLACK);
	
		actDateToShow->setFreeFont(DefaultFont);
		actDateToShow->setTextSize(1);	
		actDateToShow->setTextColor(TFT_YELLOW);
	
		actDateToShow->drawString(strDate, 0, (spriteHigth / 2) - 10, 1);
		actDateToShow->pushSprite(spriteXPos1, spriteYPos1);
		strDateOld == strDate;
	}
}

void clDisplay::fillSpriteBlack(TFT_eSprite* pSprite) {
	pSprite->fillSprite(TFT_BLACK);
}	

// -----------------------------------------------------------------------------------------------------
// show time at the middel of the display
// -----------------------------------------------------------------------------------------------------
// The display is only updated when the seconds are at 0, so a new minute starts. 
// First thing to do is completely delete the old time string by writing the string 
// with set the text color to the background color.
// Then we can write the new time string to the sprite object.
// At last the sprite object is shown at the display.
// -----------------------------------------------------------------------------------------------------
void clDisplay::showTime(struct tm _actTimeinfo, bool _bForce) {
	char str[8];

	String strZeit;
	
	switch (u16StateShowTime) {
		case 0:
			if ((_actTimeinfo.tm_sec == 0) || _bForce || (tmTimeOld.tm_min != _actTimeinfo.tm_min)) {
				// clear actual time
				fillSpriteBlack(actTimeToShow);

				// build new time string
				snprintf_P(str, sizeof(str), PSTR("%u:%02u"), _actTimeinfo.tm_hour, _actTimeinfo.tm_min);
				strZeit = String(str);
				actTimeToShow->setTextSize(1);	
				actTimeToShow->setFreeFont(TimeFont);
				actTimeToShow->setTextColor(TFT_YELLOW);
				actTimeToShow->drawCentreString(strZeit, getSpriteWidth() / 2, 0, 1);
				actTimeToShow->pushSprite(spriteXPos, spriteYPos);

				tmTimeOld = _actTimeinfo;

				u16StateShowTime = 10;
			}
			break;
		case 10: 	// wait for seccond != 0
			if (_actTimeinfo.tm_sec != 0) {
				u16StateShowTime = 0;
			}
			break;
		default:
			u16StateShowTime = 0;
			break;
	}
}
