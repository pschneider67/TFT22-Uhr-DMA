/*
 * TFT22_Uhr.h
 *
 *  Created on: 15.06.2020
 *      Author: pschneider
 */

#pragma once

enum class POLARITY: uint16_t {
	NEG = 0,
	POS = 1
};

enum class GBL_STATUS: uint16_t {
	SLEEP = 0,
	WAIT  = 1,
	RUN   = 2, 
	ERROR = 3,
	READY = 4
};

#include <WiFiManager.h> 	// https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include "psOutput.h"
#include "psInput.h"
#include "Wecker.h"
#include "Menue.h"

/*
*WM: Connection result:
*WM: 3
*WM: IP Address:
*WM: 192.168.2.10
Konfigration der Interrupts

Warte auf Zeitserver
Aktuelle Zeit wurde geladen

HTTP/1.1 200 OK
Server: openresty
Date: Mon, 15 Jun 2020 19:35:24 GMT
Content-Type: application/json; charset=utf-8
Content-Length: 444
Connection: close
X-Cache-Key: /data/2.5/weather?APPID=123456789...&cnt=2&id=2920512&lang=de&mode=json&units=metric
Access-Control-Allow-Origin: *
Access-Control-Allow-Credentials: true
Access-Control-Allow-Methods: GET, POST

{"coord":{"lon":8.65,"lat":50.58},"weather":[{"id":804,"main":"Clouds","description":"Bedeckt","icon":"04d"}],"base":"stations","main":{"temp":16.72,"feels_like":17.54,"temp_min":16.11,"temp_max":17.22,"pressure":1022,"humidity":85},"wind":{"speed":0.73,"deg":172},"clouds":{"all":100},"dt":1592249724,"sys":{"type":3,"id":2011885,"country":"DE","sunrise":1592190761,"sunset":1592249959},"timezone":7200,"id":2920512,"name":"Gießen","cod":200}
------------------------------------------------------------
Aktuelle Wetterdaten für Gießen
------------------------------------------------------------
Temperatur          : 16.72°C
Luftfeuschtigkeit   : 85.00%
Luftdruck           : 1022.00 hpa
Windgeschwindigkeit : 0.73 km/h
Beschreibung        : Bedeckt
------------------------------------------------------------
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
// Position x Mitte = (tftWidth / 2) 
// -------------------------------------		oberer Rand y = 0 bis y = 320 Pixel
// Datum und Uhrzeit
// ------------------------------------- 		yTopEnde = 40		
// -------------------------------------    	Anfang mittlerer Bereich yMiddleStart = 40 + Abstand (8)
//
//   Uhrzeit Anzeige groß
//
// -------------------------------------		Ende mittlerer Bereich yMiddleEnd = yBottumStart - Abstand (8)
// -------------------------------------		yBottumStart = tftWidth - 40
// Wlan ID und IP Adresse 
// -------------------------------------		unterer Rand y = tftWidth, x = tftHeight = 240 Pixel

// For the Adafruit shield, these are the default.
//#define TFT_DC      2         // GPIO 2  - NodeMCU D4
//#define TFT_CS      5         // GPIO 5  - NodeMCU D1
#define TFT_BACKLIGHT 0         // GPIO 0  - NodeMCU D3
#define TFT_POTI      A0        // TOUT    - NodeMCU A0

#define BUZZER        16        // GPIO 16 - NodeMCU D0
#define SW_01         12	   	// GPIO 12 - NodeMCO D6
#define SW_02         15        // GPIO 15 - NodeMCU D8
#define LED           4         // GPIO  4 - NodeMCU D2

// Falls die Anzeige gedreht werden muss
#define ROTATION_NO   0
#define ROTATION_90   1
#define ROTATION_180  2
#define ROTATION_270  3

// Prototypen
void zeigeBeschriftung(void);
void zeigeVersion(void);
void zeigeRahmen(void);
void zeigeDatumUhr(struct tm);
void zeigeUhrzeit(struct tm);
void zeigeWeckzeiten(void);
bool WeckzeitAkivieren(clIn *);
void displayHelligkeit(void);
void initGpio(void);
void initDisplay(void);
void initNetwork(void);
void wifiCallback(WiFiManager *myWiFiManager);
bool initTime(void);
void initIrq(void);
void initFs(void);
void saveConfigCallback(void);
void saveWeckerConfig(void);
void zeigeWetter(void);
GBL_STATUS getWeather(void);
GBL_STATUS getWeatherDataState (void);
void getWeatherData(void);
String WetterDatenAusJson(String WetterDaten);
void zeigeWait(void);
void zeigeText(String);

bool runHauptMenue(void); 
bool runWeckzeit1(void);
bool runWeckzeit2(void);
bool runStatus(void);
bool runWetter(void);  
bool runDeleteFile(void);

void irqTimer0(void);
void irqSw01(void);
void irqSw02(void);

String TraceTime (void);
