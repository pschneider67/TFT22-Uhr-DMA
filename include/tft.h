/*
 * tft.h
 *
 *  Created on: 24.03.2023
 *      Author: pschneider
 */

#pragma once

#define DISP_HIGTH   240
#define DISP_WIDTH   320

#define H_SPACE     8						// space
#define Y_TOP       0					 	// start upper area
#define H_TOP       35						// higth of upper area

#define H_BOTTOM    35 
#define Y_BOTTOM    (DISP_HIGTH - H_BOTTOM)  // start lower area

#define Y_MIDDLE    (H_TOP + H_SPACE)   	    // start middle area
#define H_MIDDLE    ((DISP_HIGTH - H_TOP - H_BOTTOM) - (2 * H_SPACE))  // higth of middle area

#define X_POS_WEATHER_NOW  (2 * H_SPACE)
#define Y_POS_WEATHER_NOW  (Y_BOTTOM - (H_SPACE + 2) - 64)

void showLabel(void);
void showVersion(void);
void showWeather(const char* _strIcon, uint16_t _xpos, uint16_t _ypos);
void showWeatherIcon(const unsigned short* _image, uint16_t _xpos, uint16_t _ypos);
void showState(const char*); 
void showFrame(void);