/*
 * server.cpp
 *
 *  Created on: 24.03.2023
 *      Author: pschneider
 */

#include "TFT22-Uhr.h"

static char cHtmlValuesToSend[60];
static bool bAuthentication = false;

extern ESP8266WebServer* pWifiServer;
extern std::array<clAlarm*, MAX_ALARM> Alarm;
extern stAlarmTime stWz[MAX_ALARM];

// Set the username and password for the webserver
extern const char* http_username;
extern const char* http_password;

extern bool bSaveConfigFile;

bool getAuthentication(void) {
	return bAuthentication;
}

void clearAuthentication(void) {
	bAuthentication = false;
}

bool checkAuthentication(void) {
	if (!bAuthentication) {
		Serial.println("** checkAuthentication - authentication fail");
		pWifiServer->send(200, "text/html", cHtmlAuthentication);
	} 
	return bAuthentication;
}

void handleAuthentication(void) {
	uint16_t u16Result = 0;
	Serial.println("** handleAuthentication");

	if (pWifiServer->hasArg("user")) {
		Serial.println(pWifiServer->arg("user"));
		if (strcmp(pWifiServer->arg("user").c_str(), http_username) == 0) {
			u16Result++;
		}
	}

	if (pWifiServer->hasArg("passwd")) {
		Serial.println(pWifiServer->arg("passwd"));
		if (strcmp(pWifiServer->arg("passwd").c_str(), http_password) == 0) {
			u16Result++;
		}
	}

	if (u16Result == 2) {
		bAuthentication = true;
		pWifiServer->send(200, "text/html", cHtmlConfigPage);
	} else {
		pWifiServer->send(200, "text/html", cHtmlAuthentication);
	}	
}

void handleValues() {
	if (checkAuthentication()) {
		Serial.println("** handleValues");
		char cDummy[20];

		for (int i=0; i < MAX_ALARM; i++) {
			snprintf_P(cDummy, sizeof(cDummy), PSTR("%02d:"), Alarm[i]->getAlarmHourValue());
			if (i == 0) {
				strcpy(cHtmlValuesToSend, cDummy);
			} else {	
				strcat(cHtmlValuesToSend, cDummy);
			}
			snprintf_P(cDummy, sizeof(cDummy), PSTR("%02d,"), Alarm[i]->getAlarmMinuteValue());
			strcat(cHtmlValuesToSend, cDummy);
			snprintf_P(cDummy, sizeof(cDummy), PSTR("%d,"), Alarm[i]->getAlarmWeekDayValue());
			strcat(cHtmlValuesToSend, cDummy);
			snprintf_P(cDummy, sizeof(cDummy), PSTR("%d"), Alarm[i]->getStatus());
			strcat(cHtmlValuesToSend, cDummy);
			if (i < (MAX_ALARM - 1)) {
				strcat(cHtmlValuesToSend, ",");
			}
		}
		Serial.println(cHtmlValuesToSend);
		pWifiServer->send(200, "text/plain", String(cHtmlValuesToSend));
	}
}

void handleIndex() {
	if (checkAuthentication()) {
		Serial.println("** handleIndex");
		pWifiServer->send(200, "text/html", cHtmlConfigPage);
	} 
}

void handleConfig() {
	if (checkAuthentication()) {
		Serial.println(".. handleConfig");
		String stValueName;
		bool bSaveData = false;
		bool bSaveAll = false;

		if (pWifiServer->hasArg("httpSaveAll")) {
			bSaveAll = true;
		}

		for (int i=0; i < MAX_ALARM; i++) {
			stValueName = String("httpB") + String(i);
			if (pWifiServer->hasArg(stValueName) || bSaveAll) {
				Serial.println("set alarm");	

				stValueName = String("httpWz") + String(i);
				Serial.println(stValueName);
				if (pWifiServer->hasArg(stValueName)) {
					Alarm[i]->setNewAlarmHour(pWifiServer->arg(stValueName).substring(0,2));
					Alarm[i]->setNewAlarmMinute(pWifiServer->arg(stValueName).substring(3));
					bSaveData = true;
				} else {
					bSaveData = false;
				}
				stValueName = String("httpTage") + String(i);
				Serial.println(stValueName);
				if (pWifiServer->hasArg(stValueName)) {
					Alarm[i]->setNewWeekDay(pWifiServer->arg(stValueName).substring(0,2));
				}
				stValueName = String("httpAktiv") + String(i);
				Serial.println(stValueName);
				if (pWifiServer->hasArg(stValueName)) {
					Alarm[i]->Start();
					Serial.println((String)".. Wecker " + String(i) + (String)" ist aktiv");
				} else if (bSaveData) {
					Serial.println((String)".. Alarm " + String(i) + (String)" ist nicht aktiv");
					Alarm[i]->Stop();	
				}
			}

			if (i == (MAX_ALARM - 1)) {
				bSaveConfigFile = true;
				//saveAlarmConfig();
			}
		}

		pWifiServer->send(200, "text/html", cHtmlSave);
	}
}

void handleDelete() {
	if (checkAuthentication()) {
		Serial.println("reset alarm");	
		LittleFS.remove("/config.json");

		for (int i=0; i < MAX_ALARM; i++) {
			Alarm[i]->setTime(&stWz[i]);
		}
		
		bSaveConfigFile = true;
		readConfigFile();
		pWifiServer->send(200, "text/html", cHtmlDelete);
	}
}

void handleWeather(void) {
    pWifiServer->send(200, "text/html", cHtmlWeather);
}

void handleLogout(void) {
	bAuthentication = false;
	pWifiServer->send(200, "text/html", cHtmlLogout);
}