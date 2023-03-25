/*
 * server.cpp
 *
 *  Created on: 24.03.2023
 *      Author: pschneider
 */

#include "TFT22-Uhr.h"

char cHtmlValuesToSend[60];

extern ESP8266WebServer wifiServer;
extern std::array<clAlarm, MAX_WECKER> Wecker;
extern stAlarmTime stWz[MAX_WECKER];

void handleValues() {
	Serial.println("** handleValues");
	char cDummy[20];

	for (int i=0; i < MAX_WECKER; i++) {
		snprintf_P(cDummy, sizeof(cDummy), PSTR("%02d:"), Wecker[i].getWeckStundeValue());
		if (i == 0) {
			strcpy(cHtmlValuesToSend, cDummy);
		} else {	
			strcat(cHtmlValuesToSend, cDummy);
		}
		snprintf_P(cDummy, sizeof(cDummy), PSTR("%02d,"), Wecker[i].getWeckMinuteValue());
		strcat(cHtmlValuesToSend, cDummy);
		snprintf_P(cDummy, sizeof(cDummy), PSTR("%d,"), Wecker[i].getWeckWeekDayValue());
		strcat(cHtmlValuesToSend, cDummy);
		snprintf_P(cDummy, sizeof(cDummy), PSTR("%d"), Wecker[i].getStatus());
		strcat(cHtmlValuesToSend, cDummy);
		if (i < (MAX_WECKER - 1)) {
			strcat(cHtmlValuesToSend, ",");
		}
	}
	Serial.println(cHtmlValuesToSend);
  	wifiServer.send(200, "text/plain", String(cHtmlValuesToSend));
}

void handleIndex() {
	Serial.println("** handleIndex");
	wifiServer.send(200, "text/html", cHtmlMessage);
}

void handleConfig() {
	Serial.println("** handleConfig");
	String stValueName;
	bool bSaveData = false;
	bool bSaveAll = false;

	if (wifiServer.hasArg("httpSaveAll")) {
		bSaveAll = true;
	}

	for (int i=0; i < MAX_WECKER; i++) {
		stValueName = String("httpB") + String(i);
		if (wifiServer.hasArg(stValueName) || bSaveAll) {
			Serial.println("set alarm");	

			stValueName = String("httpWz") + String(i);
			Serial.println(stValueName);
			if (wifiServer.hasArg(stValueName)) {
				Wecker[i].setNewAlarmHour(wifiServer.arg(stValueName).substring(0,2));
				Wecker[i].setNewAlarmMinute(wifiServer.arg(stValueName).substring(3));
				bSaveData = true;
			} else {
				bSaveData = false;
			}
			stValueName = String("httpTage") + String(i);
			Serial.println(stValueName);
			if (wifiServer.hasArg(stValueName)) {
				Wecker[i].setNewWeekDay(wifiServer.arg(stValueName).substring(0,2));
			}
			stValueName = String("httpAktiv") + String(i);
			Serial.println(stValueName);
			if (wifiServer.hasArg(stValueName)) {
				Wecker[i].Start();
				Serial.println((String)".. Wecker " + String(i) + (String)" ist aktiv");
			} else if (bSaveData) {
				Serial.println((String)".. Wecker " + String(i) + (String)" ist nicht aktiv");
				Wecker[i].Stop();	
			}
		}

		if (i == (MAX_WECKER - 1)) {
			saveWeckerConfig();
		}
	}

	wifiServer.send(200, "text/html", cHtmlSave);
	stValueName = String("httpReset");
}

void handleDelete() {
	Serial.println("reset alarm");	
	LittleFS.remove("/config.json");

	for (int i=0; i < MAX_WECKER; i++) {
		Wecker[i].setTime(&stWz[i]);
	}
	
	saveWeckerConfig();
	initFs();
	wifiServer.send(200, "text/html", cHtmlDelete);
}

void handleWeather(void) {
    wifiServer.send(200, "text/html", cHtmlWeather);
}