/*
 * ota.cpp
 *
 *  Created on: 24.03.2023
 *      Author: pschneider
 */

#include "TFT22-Uhr.h"

extern TFT_eSPI tft;
extern const GFXfont *DefaultFont;

// -----------------------------------------------------------------------------------
// handle OTA
// -----------------------------------------------------------------------------------
void initOTA(void) {
	Serial.println("-- init OTA");
	// set callback functions
	ArduinoOTA.onStart(cbOtaOnStart);
	ArduinoOTA.onEnd(cbOtaOnEnd);
	ArduinoOTA.onProgress(cbOtaOnProgress);
	ArduinoOTA.onError(cbOtaOnError);
	ArduinoOTA.begin(); 	
}

void cbOtaOnStart(void) {
	tft.fillScreen(TFT_BLACK);
	tft.setFreeFont(DefaultFont);
	tft.setTextColor(TFT_WHITE, TFT_BLACK);
	tft.setCursor(0,30);
	tft.println(F(".. Start Update"));
}

void cbOtaOnEnd(void) {
	tft.println(F(".. Restart System"));
	delay(3000);
}

void cbOtaOnProgress(unsigned int progress, unsigned int total) {
	static uint16_t u16FirstCall = true;
	static uint16_t u16Count = 0;

	if (u16FirstCall) {
		tft.print(F(".. Progress: "));
		u16FirstCall = false;
	} else {
		if (u16Count++ == 25) {
			tft.print(F("."));
			u16Count = 0;
		}
	}

	if (progress == total) {
		tft.println();
		tft.println(F(".. Data sent ready"));
	}
}

void cbOtaOnError(ota_error_t error) {
	tft.println();
    Serial.printf("Fehler beim OTA-Update [%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
		tft.println(F(".. Authentifizierungsfehler"));
	} else if (error == OTA_BEGIN_ERROR) {
		tft.println(F(".. Fehler beim Starten des OTA-Updates"));
	} else if (error == OTA_CONNECT_ERROR) {
		tft.println(F(".. Verbindungsfehler"));
	} else if (error == OTA_RECEIVE_ERROR) {
		tft.println(F(".. Empfangsfehler"));
    } else if (error == OTA_END_ERROR) {
		tft.println(F(".. Fehler beim Beenden des OTA-Updates"));
	} else {
		tft.println(F(".. OTA Fehler"));
	}
	delay(3000);
}
