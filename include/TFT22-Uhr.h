/*
 * TFT22_Uhr.h
 *
 *  Created on: 15.06.2020
 *      Author: pschneider
 */

#pragma once

#include "Arduino.h"

#include <string.h>
#include <FS.h>
#include <time.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h> 		// OTA Upload via ArduinoIDE
#include <psGpio.h>
#include <WiFiManager.h> 		// https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include "psDisplay.h"
#include "psWecker.h"
#include "psMenue.h"

#include "html.h"
#include "server.h"

// For the Adafruit shield, these are the default.
#define TFT_BACKLIGHT 0         // GPIO 0  - NodeMCU D3
#define TFT_POTI      A0        // TOUT    - NodeMCU A0

#define BUZZER        16        // GPIO 16 - NodeMCU D0
#define SW_01         15        // GPIO 15 - NodeMCU D8
#define SW_02         12	   	  // GPIO 12 - NodeMCO D6
#define LED           4         // GPIO  4 - NodeMCU D2

#define PWM_FREQ      1000
#define PWM_MAX       1023    

// to rotate the screen
#define ROTATION_NO   0
#define ROTATION_90   1
#define ROTATION_180  2
#define ROTATION_270  3

void showTime(struct tm, bool);
void showAlarmTime(bool);
void showLabel(void);

bool enableAlarmTime(clIn *);
void tftBrigthnees(void);

void initGpio(void);
void initDisplay(void);
void initNetwork(void);

void initIrq(void);
void readConfigFile(void);

String getJsonsDataFromWeb (String, String);

void wifiCallbackSaveConfig(void);
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
void irq(void);

void getActualWeather(void);
void decodeCurrentWeather(String WetterDaten);
void getWeatherForcast(void);
void decodeWeatherForcast(String WetterDaten);

String convertStringToGerman(String);
String TraceTime (void);
