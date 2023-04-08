/*
 * TFT22_Uhr.h
 *
 *  Created on: 15.06.2020
 *      Author: pschneider
 */

#pragma once

#include "Arduino.h"

#include <string.h>
#include <time.h>
#include <LittleFS.h>
#include <SPI.h>
#include <TFT_eSPI.h> 		
#include <TimeLib.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h> 		// OTA Upload via ArduinoIDE
#include <psGpio.h>
#include <WiFiManager.h> 		// https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include "font.h"
#include "psWecker.h"
#include "psMenue.h"

#include "html.h"
#include "ota.h"
#include "tft.h"
#include "server.h"

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

// For the Adafruit shield, these are the default.
//#define TFT_DC      2        	// GPIO 2  - NodeMCU D4
//#define TFT_CS      5        	// GPIO 5  - NodeMCU D1
#define TFT_BACKLIGHT 0         // GPIO 0  - NodeMCU D3
#define TFT_POTI      A0        // TOUT    - NodeMCU A0

#define BUZZER        16        // GPIO 16 - NodeMCU D0
#define SW_01         12	   	// GPIO 12 - NodeMCO D6
#define SW_02         15        // GPIO 15 - NodeMCU D8
#define LED           4         // GPIO  4 - NodeMCU D2

// to rotate the screen
#define ROTATION_NO   0
#define ROTATION_90   1
#define ROTATION_180  2
#define ROTATION_270  3

void showDateAndTime(struct tm);
void showTime(struct tm, bool);
void showAlarmTime(bool);

bool enableAlarmTime(clIn *);
void tftBrigthnees(void);

void initGpio(void);
void initDisplay(void);
void initNetwork(void);

bool initTime(void);
void initIrq(void);
void initFs(void);

String getJsonsDataFromWeb (String, String);

void saveConfigCallback(void);
void wifiCallback(WiFiManager *myWiFiManager);

void saveWeckerConfig(void);
void showWeatherString(void);
bool changeAlarmTime(void);  

bool runMainMenue(void);
bool runState(void);
bool runDeleteFile(void);
bool runWeatherForcast(void);

void irqTimer0(void);
void irqSw01(void);
void irqSw02(void);

void getActualWeather(void);
void decodeCurrentWeather(String WetterDaten);
void getWeatherForcast(void);
void decodeWeatherForcast(String WetterDaten);

String convertStringToGerman(String);
String TraceTime (void);
