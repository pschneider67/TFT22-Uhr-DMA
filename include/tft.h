/*
 * tft.h
 *
 *  Created on: 24.03.2023
 *      Author: pschneider
 */

#pragma once

void showLabel(void);
void showVersion(void);
void showWeather(const char* _strIcon, uint16_t _xpos, uint16_t _ypos);
void showWeatherIcon(const unsigned short* _image, uint16_t _xpos, uint16_t _ypos);
void showState(const char* _strData);
