/*
 * tft.cpp
 *
 *  Created on: 24.03.2023
 *      Author: pschneider
 */

#include "TFT22-Uhr.h"

extern char cVersion[] PROGMEM;
extern char cDatum[]   PROGMEM;

extern TFT_eSPI tft;

extern TFT_eSprite actTimeToShow;
extern TFT_eSprite actDateToShow;
extern TFT_eSprite actTimeSecToShow;

extern const GFXfont *DefaultFont;
extern const GFXfont *TimeFont;

void showLabel(void) {
	char strText[40];
	char strIp[16];
	WiFi.localIP().toString().toCharArray(strIp, 16);
	snprintf_P(strText, sizeof(strText), PSTR("%s - %s"), WiFi.SSID().c_str(), strIp);
	showState(strText);
}

void showVersion(void) {
	char strText[40];
	snprintf_P(strText, sizeof(strText), PSTR("Ver. %s - %s"), cVersion, cDatum); 
	showState(strText);
}

void showWeather(const char* _strIcon, uint16_t _xpos, uint16_t _ypos) {
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

void showWeatherIcon(const unsigned short* _image, uint16_t _xpos, uint16_t _ypos) {
	uint16_t u16Width = 64;		// icon size
	uint16_t u16Higth = 64;
	
	tft.fillRect(_xpos, _ypos, u16Width, u16Higth, TFT_BLACK);	
	tft.pushImage(_xpos, _ypos, 64, 64, _image);
}

void showState(const char* _strData) {
	static char strOld[40] = {"                                       "};
			
	tft.setFreeFont(DefaultFont);
	tft.setTextSize(1);	

	// clear actual string
	tft.setTextColor(TFT_BLACK, TFT_BLACK);
	tft.drawCentreString(strOld, DISP_WIDTH / 2, Y_BOTTOM + 8, 1);

	// draw new string
	tft.setTextColor(TFT_YELLOW, TFT_BLACK);
	tft.drawCentreString(_strData, DISP_WIDTH / 2, Y_BOTTOM + 8, 1);
	tft.setFreeFont(NULL);

	strcpy(strOld, _strData);
}

void showFrame(void) {
	tft.fillScreen(TFT_BLACK);
	tft.drawRoundRect(1, Y_TOP, DISP_WIDTH - 1, H_TOP, 10, TFT_BLUE);
	tft.drawRoundRect(1, Y_MIDDLE, DISP_WIDTH - 1, H_MIDDLE, 10, TFT_BLUE);
	tft.drawRoundRect(1, Y_BOTTOM, DISP_WIDTH - 1, H_BOTTOM, 10, TFT_BLUE);
}