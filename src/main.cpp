// -----------------------------------------------------------------------------------
// Autor : Peter Schneider
// Datum : 23.01.2021 - Wechsel auf VSCode - PlatformIO
//
// D1_mini mit TFT 2.2", 320 x 240, SPI
// -----------------------------------------------------------------------------------
// Pinbelegung
// -----------------------------------------------------------------------------------
// VCC        - 3V3
// GND        - GND
// CS         -  D1   - GPIO-05
// RESET      - RST
// D/C        -  D4   - GPIO-02
// SDI / MOSI -  D7   - GPIO-13
// SCK        -  D5   - GPIO-14
// LED        -  D3   - GPIO-00 Hintergrundbeleuchtung an PWM
// SDO / MISO -  D6   - GPIO-12 Achtung Doppelbelegung mit Taster 1
// -----------------------------------------------------------------------------------
// LDR        -  A0   - ADC0    TFT Hintergrudbeleuchtung
// Buzzer     -  D0	  - GPIO-16
// Taster 1   -  D6	  - GPIO-12
// Taster 2   -  D8   - GPIO-15
// LED        -  D2   - GPIO-04
// -----------------------------------------------------------------------------------
#include "Arduino.h"

#include <string.h>
#include <time.h>
#include <LittleFS.h>
#include <WiFiManager.h> 	// https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <SPI.h>
#include <TFT_eSPI.h> 		// Hardware-specific library
#include <TimeLib.h>
#include <ArduinoJson.h>

#include "Free_Fonts.h"
#include "TFT22-Uhr.h"

// ---------------------------------------------------------------------------------------------------
// in dieser Datei steht das define für API_KEY. Die Datei muss noch angelegt werden,
// sie ist nicht Bestandteil des Repository
// ---------------------------------------------------------------------------------------------------
#include "config.h"
// ---------------------------------------------------------------------------------------------------

#define MY_TZ "CET-1CEST,M3.5.0,M10.5.0/3"			// Zeitformat mit Sommerzeit / Winterzeit
#define MY_NTP_SERVER "europe.pool.ntp.org" 		// verwendeter Zeitserver

time_t actualTime;
struct tm timeinfo;

TFT_eSPI tft = TFT_eSPI();

// use openweather setup
String ApiKey = API_KEY;
String CityId = "2920512"; // Giessen
const char Server[] = "api.openweathermap.org";
StaticJsonDocument<2000> doc; 						 // JSON Dokument erstellen

const char *WeekDay[7] = {"So. ", "Mo. ", "Di. ", "Mi. ", "Do. ", "Fr. ", "Sa. "};

const uint16_t hSpace = 8;							 // Abstand
const uint16_t yTop = 0;							 // Start oberer Bereich
const uint16_t hTop = 40;							 // Höhe des oberen Bereiches
const uint16_t yMiddle = hTop + hSpace;				 // Start mittlerer Bereich
const uint16_t hMiddle = yMiddle + 95;				 // Höhe des ittleren Bereiches
const uint16_t yBottom = yMiddle + hMiddle + hSpace; // Start des unteren Bereiches
const uint16_t hBottom = 40;						 // Höhe des unteren Bereiches

uint16_t tftWidth;
uint16_t tftHeight;

WiFiManager wm;
WiFiClient client;

clOut led;
clOut buzzer;

stInput ParamSw01; 	// Eingangsparameter
stInput ParamSw02;
clIn sw01; 			// Eingangsklasse
clIn sw02;

char cVersion[] = {"01.00"};
char cDatum[]   = __DATE__;

// Weckzeit 1
stWeckZeit WeckZeit1 = {WOCHEN_TAG::AT, 5, 55};
clWecken Wecker1(&timeinfo, &buzzer, &sw02);
char Wecker1_Stunden[3] = {"00"};
char Wecker1_Minuten[3] = {"00"}; 
char Wecker1_Aktiv[2] = {" "};

// Weckzeit 2
stWeckZeit WeckZeit2 = {WOCHEN_TAG::WE, 21, 00};
clWecken Wecker2(&timeinfo, &buzzer, &sw02);
char Wecker2_Stunden[3] = {"00"};
char Wecker2_Minuten[3] = {"00"};
char Wecker2_Aktiv[2] = {" "};

bool shouldSaveConfig = false;

// Menueverwaltung
clMenue Menue(&sw01, zeigeText);
menue_t MenueEintrag [6] = { 
	{runHauptMenue, String("Uhrzeit / Weckzeiten"), false},
	{runWeckzeit1,  String("Weckzeit 1 stellen"),   false},
	{runWeckzeit2,  String("Weckzeit 2 stellen"),   false},
	{runStatus,     String("Statusanzeige"),        false},
	{runWetter,     String("Wetteranzeige"),        false},
	{runDeleteFile, String("Delete Konfigiration"),  true}
};

int pwmValue;

// Daten vom JSON-Tree abrufen
char *cityData;
float tempNow;
float tempMin;
float tempMax;
float humidityNow;
float pressureNow;
char *weatherNowData;

// ---------------------------------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------------------------------
void setup() {
	Serial.begin(115200);

	initDisplay();
	initGpio();
	initNetwork();
	
	displayHelligkeit();
	zeigeRahmen();
	zeigeBeschriftung();

	configTime(MY_TZ, MY_NTP_SERVER);

	initFs();	// lese Konfig Daten für die Wecker

	Wecker1.setTime(&WeckZeit1);
	Wecker2.setTime(&WeckZeit2);

	Serial.println();
	Serial.println("--------------------------------------");
	Serial.println("- TFT2.2 Uhr SPI                     -");
	Serial.println("--------------------------------------");
	Serial.println(cVersion);
	Serial.println(cDatum);

	initIrq();
}

// ---------------------------------------------------------------------------------------------------
// Loop - Endlosschleife
// ---------------------------------------------------------------------------------------------------
void loop(void) {
	led.SwPort(sw01.Status());				// LED an wenn Taste 1 betätigt

	time(&actualTime);					 	// aktuelle Zeit lesen
	localtime_r(&actualTime, &timeinfo); 	// timeinfo mit aktueller Zeit beschreiben

	Wecker1.Check();						// Weckzeit prüfen und ggf. Wecken
	Wecker2.Check();

	zeigeUhrzeit(timeinfo);	 				// Uhrzeit groß anzeigen
	zeigeDatumUhr(timeinfo); 				// Datum und Uhrzeit in oberer Zeile anzeigen
	displayHelligkeit();
	zeigeWeckzeiten();
	
	Menue.Verwaltung(MenueEintrag);			// Abarbeiten der Menüs
}

// ---------------------------------------------------------------------------------------------------
// Aktiviere Weckzeiten mit Taster 
// Zum Starten muss die Taste 5sec lang gedückt werden
// ---------------------------------------------------------------------------------------------------
// Tastendruck 		Wecker 1		Wecker 2
//       1             1               0
//       2             0               1
//       3             1               1
//       4             0               0
// ---------------------------------------------------------------------------------------------------
bool WeckzeitAkivieren(clIn *Taste) {
	static uint16_t u16Status = 0;
	static uint16_t u16StateOld = 1;
	static uint16_t u16Count = 0;
	static uint32_t u32AktuelleZeit = 0;
	static uint32_t u32AktiveZeit = 0;

	bool bResult = false;

	if (u16Status != u16StateOld)
	{
		Serial.println(TraceTime() + "WA Status - " + String(u16Status));
		u16StateOld = u16Status;
	}

	switch(u16Status) {
		case 0:
			bResult = true;
			if (Taste->Status()) {
				u32AktuelleZeit = millis();
				u16Status = 10;
			}
			break;
		case 10:
			if ((millis() - u32AktuelleZeit) >= 3000) {
				if (Taste->Status()) {
					if (Wecker1.GetStatus() && Wecker2.GetStatus()) {
						u16Count = 3;
					} else if (Wecker1.GetStatus()) {
						u16Count = 1;
					} else if (Wecker2.GetStatus()) {
						u16Count = 2;
					} else {
						u16Count = 0;
					}
					u32AktiveZeit = millis();
					u16Status = 20;
				}
			} else if (!Taste->Status()) {
				u16Status = 0;
			}
			break;
		case 20:
			if (Taste->Status()) {
				switch(u16Count) {
					case 0: 
						Wecker1.Start(); 
						Wecker2.Stop();  
						sprintf(Wecker1_Aktiv, "*");
						sprintf(Wecker2_Aktiv, " ");
						u16Count++;   
						break;
					case 1: 
						Wecker1.Stop();  
						Wecker2.Start(); 
						sprintf(Wecker1_Aktiv, " ");
						sprintf(Wecker2_Aktiv, "*");
						u16Count++;   
						break;
					case 2:	
						Wecker1.Start(); 
						Wecker2.Start(); 
						sprintf(Wecker1_Aktiv, "*");
						sprintf(Wecker2_Aktiv, "*");
						u16Count++;   
						break;
					case 3:	
						Wecker1.Stop();  
						Wecker2.Stop();  
						sprintf(Wecker1_Aktiv, " ");
						sprintf(Wecker2_Aktiv, " ");
						u16Count = 0; 
						break;
					default: 
						u16Count = 0; 
						break;				
				}
				u32AktuelleZeit = millis();
				u16Status = 30;
			}

			if (millis() > (u32AktiveZeit + 2000)) {
				saveWeckerConfig();
				u16Status = 0;
			}
			break;
		case 30:
			if (!Taste->Status()) {
				u32AktiveZeit = millis();
				u16Status = 20;
			}
			break;
		default:
			u16Status = 0;
			break;
	}
	return bResult;
}

// ---------------------------------------------------------------------------------------------------
// Display Funktionen
// ---------------------------------------------------------------------------------------------------
void zeigeBeschriftung(void) {
	String strText = WiFi.SSID();
	strText += String(" - ");
	strText += WiFi.localIP().toString(); 

	tft.fillRect(5, yBottom + 10, tftWidth - 10, hBottom - 20, TFT_BLACK);
	tft.setTextColor(TFT_YELLOW);
	tft.drawCentreString(strText, tftWidth / 2, yBottom + 12, 1);
}

void zeigeVersion(void) {
	String strText = String("Ver. "); 
	strText += String(cVersion);
	strText += String(" - ");
	strText += String(cDatum); 

	tft.fillRect(5, yBottom + 10, tftWidth - 10, hBottom - 20, TFT_BLACK);
	tft.setTextColor(TFT_YELLOW);
	tft.drawCentreString(strText, tftWidth / 2, yBottom + 12, 1);
}

// ---------------------------------------------------------------------------------------------------
// Funktionen zu den Menüpunkten
// ---------------------------------------------------------------------------------------------------
bool runHauptMenue(void) {
	return WeckzeitAkivieren(&sw02);
} 

bool runWeckzeit1(void) {
	static uint16_t u16Status = 0;
	
	bool bResult = false;

	switch (u16Status) {
		case 0:		// warte bis Tatse losgelassen wurde
			if (!sw02.Status()) {	
				u16Status = 5;
			}
			break;
		case 5:		// warte auf Tastendruck
			bResult = true;	
			if (sw02.Status()) {	
				u16Status = 10;
			}
			break;
		case 10:	
			if (Wecker1.stelleWeckzeit(&WeckZeit1)) {
				Wecker1.getWeckStunde().toCharArray(Wecker1_Stunden, 3);   
				Wecker1.getWeckMinute().toCharArray(Wecker1_Minuten, 3);   
				saveWeckerConfig();
				bResult = true;
				u16Status = 0;
			} 
			break;
		default:
			u16Status = 0;
			break;	
	}
	return bResult;
}

bool runWeckzeit2(void) {
	static uint16_t u16Status = 0;
	
	bool bResult = false;

	switch (u16Status) {
		case 0:		// warte bis Tatse losgelassen wurde
			if (!sw02.Status()) {	
				u16Status = 5;
			}
			break;
		case 5:		// warte auf Tastendruck
			bResult = true;	
			if (sw02.Status()) {	
				u16Status = 10;
			}
			break;
		case 10:	
			if (Wecker2.stelleWeckzeit(&WeckZeit2)) {
				Wecker2.getWeckStunde().toCharArray(Wecker2_Stunden, 3);   
				Wecker2.getWeckMinute().toCharArray(Wecker2_Minuten, 3);   
				saveWeckerConfig();
				bResult = true;
				u16Status = 0;
			} 
			break;
		default:
			u16Status = 0;
			break;	
	}
	return bResult;
}

bool runStatus(void) {
	static uint16_t u16Status = 0;
	static uint32_t u32Timer = 0;
	bool bResult = false;

	switch (u16Status) {
		case 0:
			bResult = true;
			if (sw02.Status()) {
				u16Status = 10;
			}
			break;
		case 10:
			zeigeBeschriftung();
			u32Timer = millis();
			u16Status = 20;
			break;
		case 20:
			if (millis() > (u32Timer + 4000)) {
				u16Status = 30;
			}
			break;
		case 30:
			zeigeVersion();	
			u32Timer = millis();
			u16Status = 40;
			break;
		case 40:
			if (millis() > (u32Timer + 4000)) {
				u16Status = 0;
			}
			break;
		default:
			u16Status = 0;
			break;
	}
	
	return bResult;
}

bool runWetter(void) {
	static uint16_t u16Status = 0;
	static uint32_t u32Timer = 0;
	bool bResult = false;
	GBL_STATUS weatherState;

	switch (u16Status) {
		case 0:
			bResult = true;
			if (sw02.Status()) {
				zeigeWait();
				u16Status = 10;
			}
			break;
		case 10:
			if (!sw02.Status()) {
				u16Status = 20;
			}
			break; 
		case 20:
			weatherState = getWeatherDataState();
			if (weatherState == GBL_STATUS::ERROR) {
				Serial.println(TraceTime() + "getWeatherDataState Error");
				u16Status = 0;
			} else if (weatherState == GBL_STATUS::READY) {
				zeigeWetter();
				u32Timer = millis();
				u16Status = 30;
			}
			break;
		case 30:
			if (millis() > (u32Timer + 5000)) {
				u16Status = 0;
			}
			break;
		default:
			u16Status = 0;
			break;
	}

	return bResult;
}  

bool runDeleteFile(void) {
	static uint16_t u16Status = 0;
	static uint32_t u32Timer = 0;
	bool bResult = false;
	char cFile[] = "/config.json";
	
	switch (u16Status) {
		case 0:
			bResult = true;
			if (sw02.Status()) {
				Serial.printf("Deleting file: %s\r\n", cFile);
				u16Status = 10;
			}
			break;
		case 10:
			if (LittleFS.remove(cFile)) {
				zeigeText(String("Konfig. deleted"));
			} else {
				zeigeText(String("Fehler bei delete"));
			}
			u32Timer = millis();
			u16Status = 20;
			break;
		case 20:
			if (millis() > (u32Timer + 3000)) {
				u16Status = 0;
			}
			break;
		default:
			bResult = true;
			u16Status = 0;
				break;
	}
	
	return bResult;
}

// -----------------------------------------------------------------------------------
// Wetter Anzeige
// -----------------------------------------------------------------------------------
void zeigeWetter(void) {
	String strText = String(tempNow) + String(char(247)) + String("C, ") + String(weatherNowData);
	tft.fillRect(5, yBottom + 10, tftWidth - 10, hBottom - 20, TFT_BLACK);
	tft.setTextColor(TFT_YELLOW);
	tft.drawCentreString(strText, tftWidth / 2, yBottom + 12, 1);
}

void zeigeWait(void) {
	String strText = "Warte auf Wetterdaten";
	tft.fillRect(5, yBottom + 10, tftWidth - 10, hBottom - 20, TFT_BLACK);
	tft.setTextColor(TFT_YELLOW);
	tft.drawCentreString(strText, tftWidth / 2, yBottom + 12, 1);
}

void zeigeText(String strText) {
	tft.fillRect(5, yBottom + 10, tftWidth - 10, hBottom - 20, TFT_BLACK);
	tft.setTextColor(TFT_YELLOW);
	tft.drawCentreString(strText, tftWidth / 2, yBottom + 12, 1);
}

void zeigeRahmen(void) {
	tft.fillScreen(TFT_BLACK);
	tft.drawRoundRect(1, yTop, tftWidth - 1, hTop, 10, TFT_BLUE);
	tft.drawRoundRect(1, yMiddle, tftWidth - 1, hMiddle, 10, TFT_BLUE);
	tft.drawRoundRect(1, yBottom, tftWidth - 1, hBottom, 10, TFT_BLUE);
}

void zeigeDatumUhr(struct tm actTimeinfo) {
	char str[30];
	static String strZeitOld = " ";
	String strZeit;

	strZeit = String(WeekDay[actTimeinfo.tm_wday]);
	sprintf(str, "%02u.%02u.%4u - ", actTimeinfo.tm_mday, actTimeinfo.tm_mon, 1900 + actTimeinfo.tm_year);
	strZeit += String(str);
	sprintf(str, "%02u:%02u:%02u", actTimeinfo.tm_hour, actTimeinfo.tm_min, actTimeinfo.tm_sec);
	strZeit += String(str);

	// akuelle Zeit nur neu schreiben wenn sich die Zeit geändert hat
	if (strZeitOld != strZeit)
	{
		tft.setTextColor(TFT_YELLOW, TFT_BLACK);
		tft.drawCentreString(strZeit, tftWidth / 2, 14, 1);
		strZeitOld = strZeit;
	}
}

void zeigeWeckzeiten(void) {
	static String oldString1 = " ";
	static String oldString2 = " ";

	String Str1 = Wecker1.getTimeString();
	String Str2 = Wecker2.getTimeString();

	tft.setFreeFont(NULL);

	if (oldString1 != Str1) {
		tft.drawString(Str1, 20, 145);
		oldString1 = Str1;
	}
	if (oldString2 != Str2) {
		tft.drawString(Str2, 20, 165);
		oldString1 = Str2;
	}
}

// -----------------------------------------------------------------------------------------------------
// Zeitanzeige in der Mitte (groß)
// -----------------------------------------------------------------------------------------------------
// Die Anzeige wird nur aktuallisiert wenn die Sekunden auf 0 stehen,
// also eine neue Minute anfängt. Da als erstes der alte Schriftzug komplett gelöscht wird
// bevor die aktuelle Zeit angezeigt wird, kommt es zu einem kurzen Flackern.
// -----------------------------------------------------------------------------------------------------
void zeigeUhrzeit(struct tm actTimeinfo) {
	static uint16_t u16State = 0;
	char str[10];
	String strZeit;

	switch (u16State)  	{
		case 0: 	// Zeitanzeige initialisieren wenn die NTP Abfrage zum 1. mal komplett ist
			tft.setFreeFont(NULL);
			tft.drawCentreString("NTP Abfrage", tftWidth / 2, 145, 1);
			u16State = 5;
			break;
		case 5: 	// Abfrage ob Jahr > 2000 (1900 + 100) ist
			if (actTimeinfo.tm_year > 100) {
				u16State = 20;
			} else {
				u16State = 6;
			}
			break;
		case 6: 	// warte 100ms
			delay(100);
			u16State = 5;
			break;
		case 10: 	// Warte auf den Anfang einer neuen Minute (Sekunde == 0)
			if (actTimeinfo.tm_sec == 0) {
				u16State = 20;
			}
			break;
		case 20: 	// aktuelle Zeit ausgeben
			tft.setFreeFont(FF20);
			tft.setTextColor(TFT_YELLOW, TFT_BLACK);
			sprintf(str, "%u:%02u", actTimeinfo.tm_hour, actTimeinfo.tm_min);
			if (actTimeinfo.tm_hour == 0) {
				strZeit = String(" ") + String(str) + String(" ");
			} else {
				strZeit = String(str);
			}
			
			tft.fillRect(40, yMiddle + 20, tftWidth - 80, hMiddle - 50, TFT_BLACK);
			tft.drawCentreString(strZeit, tftWidth / 2, (tftHeight / 2) - 60, 1);

			u16State = 30;
			break;
		case 30: 	// wrate bis Skunden != 0
			if (actTimeinfo.tm_sec != 0) {
				u16State = 10;
			}
			break;
		default:
			u16State = 0;
			break;
	}
}

// -------------------------------------------------------------------------------------------------
// Display Helligkeit über LDR gsteuert
// -------------------------------------------------------------------------------------------------
void displayHelligkeit(void) {
	// pwmValue = 4 * Messwert - 68 -> linearer Zusammenhang empirisch erittelt
	pwmValue = (4 * analogRead(TFT_POTI)) - 68;		// Analogwert 0 - 1024

	if (pwmValue < 1) {pwmValue = 1;}				// untere Begrenzung auf 1
	if (pwmValue > 1024) {pwmValue = 1024;}			// oberer Begrenzung auf 1024

	analogWrite(TFT_BACKLIGHT, pwmValue);
}

// -----------------------------------------------------------------------------------
// Interrupt Funktion - Timer 0 - 1s
// -----------------------------------------------------------------------------------
// wird zur Zeit nicht verwendet
// -----------------------------------------------------------------------------------
IRAM_ATTR void irqTimer0(void) {
	//Serial.println(TraceTime() + "Timer IRQ");
	timer0_write(ESP.getCycleCount() + 80000000L); // Bei 80MHz ergibt sich damit jede Sekunde ein Interrupt
}

// -----------------------------------------------------------------------------------
// init GPIO
// -----------------------------------------------------------------------------------
void initGpio(void) {
	// Summer
	buzzer.Init(BUZZER, POLARITY::NEG); // GIPO BUZZER low active
	buzzer.On();
	delay(10);
	buzzer.Off();

	// LED
	led.Init(LED, POLARITY::POS); 		// GPIO LED high active

	// Taster SW 01
	ParamSw01.cb = irqSw01;				// Interrupt Service Routine
	ParamSw01.entprellzeit = 30;		// Entprellzeit [ms]
	ParamSw01.irq = false;				// Interrupt verwenden wenn hier true
	ParamSw01.mode = CHANGE;			// Interrupt bei Signalwechsel
	ParamSw01.pin = SW_01;				// GPIO Nummer
	ParamSw01.polarity = POLARITY::POS; // Polarität
	ParamSw01.status = false;			// Status des Eingangs, Wert ist hier egal
	sw01.Init(&ParamSw01);				// Initialisiere den Eingang

	// Taster SW 02
	ParamSw02.cb = irqSw02;				// Interrupt Service Routine
	ParamSw02.entprellzeit = 30;		// Entprellzeit [ms]
	ParamSw02.irq = false;				// Interrupt verwenden wenn hier true
	ParamSw02.mode = RISING;			// Interrupt bei Tastendruck (steigente Flanke)
	ParamSw02.pin = SW_02;				// GPIO Nummer
	ParamSw02.polarity = POLARITY::POS; // Plarität
	ParamSw02.status = false;			// Status des Eingangs, Wert ist hier egal
	sw02.Init(&ParamSw02);				// Initialisiere den Eingang

	// TFT Hintergrundbeleuchtung init und einschalten
	analogWriteRange(1024);
	analogWriteFreq(5000);
	analogWrite(TFT_BACKLIGHT, 1024); 	// Wertebereich 0 - 1024
}

// -----------------------------------------------------------------------------------
// init Display
// -----------------------------------------------------------------------------------
void initDisplay(void) {
	tft.init();
	tft.setRotation(ROTATION_90);
	tft.fillScreen(TFT_BLACK);

	tft.setTextColor(TFT_WHITE, TFT_BLACK);
	tft.setCursor(0, 30);
	tft.setTextSize(2);

	tftWidth = tft.width();
	tftHeight = tft.height();
}

// -----------------------------------------------------------------------------------
// init Netzwerk
// -----------------------------------------------------------------------------------
void initNetwork(void) {
	wm.setAPCallback(wifiCallback);
	wm.setSaveConfigCallback(saveConfigCallback);

	tft.print("Netzwerverbindung starten\n");
	if (wm.autoConnect("ESP_TFT_UHR")) {
		tft.print(".. Netzwerverbindung OK\n");
		delay(1000);
	} else {
		tft.print(".. Netzwerkfehler !!\n");
		tft.print(".. ESP startet neu !!\n");
		while (true) {;}
	}
}

void wifiCallback(WiFiManager *myWiFiManager) {
	tft.print(".. Konfig. Mode aktiv");
	tft.print("\n.. ");
	tft.print(WiFi.softAPIP());
	tft.print("\n.. ");
	tft.print(myWiFiManager->getConfigPortalSSID());
	tft.print("\n");
}

// -----------------------------------------------------------------------------------
// init File System, lese Konfiguration
// -----------------------------------------------------------------------------------
void initFs(void) {
	Serial.println(".. mounting FS");

  	if (LittleFS.begin()) {
   		Serial.println(".. mounted file system");
    	if (LittleFS.exists("/config.json")) {
      		// file exists, reading and loading
      		Serial.println(".. reading config file");
      		File configFile = LittleFS.open("/config.json", "r");
      		if (configFile) {
        		Serial.println(".. opened config file");
        		size_t size = configFile.size();
        		
				// Allocate a buffer to store contents of the file.
        		std::unique_ptr<char[]> buf(new char[size]);

        		configFile.readBytes(buf.get(), size);

		       	DynamicJsonDocument json(1024);
        		auto deserializeError = deserializeJson(json, buf.get());
        		serializeJson(json, Serial);
        		if (!deserializeError) {
          			Serial.println("\nparsed json");
          			strcpy(Wecker1_Stunden, json["Wecker1_Stunden"]);
          			strcpy(Wecker1_Minuten, json["Wecker1_Minuten"]);
					strcpy(Wecker1_Aktiv, json["Wecker1_Aktiv"]);  

          			strcpy(Wecker2_Stunden, json["Wecker2_Stunden"]);
          			strcpy(Wecker2_Minuten, json["Wecker2_Minuten"]);
					strcpy(Wecker2_Aktiv, json["Wecker2_Aktiv"]);  
					
					String Data = (String)Wecker1_Stunden;
					WeckZeit1.u16Stunde = Data.toInt();  
					Data = (String)Wecker1_Minuten;
					WeckZeit1.u16Minute = Data.toInt();
					Wecker1.setTime(&WeckZeit1);
					if (Wecker1_Aktiv[0] == '*') {
						Serial.println(".. Wecker 1 ist aktiv");
						Wecker1.Start();
					}	

					Data = (String)Wecker2_Stunden;
					WeckZeit2.u16Stunde = Data.toInt();  
					Data = (String)Wecker2_Minuten;
					WeckZeit2.u16Minute = Data.toInt();
					Wecker2.setTime(&WeckZeit2);
					if (Wecker2_Aktiv[0] == '*') {
						Serial.println(".. Wecker 2 ist aktiv");
						Wecker2.Start();
					}
          		} else {
          			Serial.println(".. failed to load json config");
        		}
        		configFile.close();
      		}
    	}
  	} else {
    	Serial.println(".. failed to mount FS");
  	}

  	Serial.println("Date aus dem Config File : ");
  	Serial.println("\tW1 : " + String(Wecker1_Stunden) + String(":") + String(Wecker1_Minuten));
  	Serial.println("\tW2 : " + String(Wecker2_Stunden) + String(":") + String(Wecker2_Minuten));
}

// callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Konfiguration sollte gespeichert werden");
  shouldSaveConfig = true;
}

void saveWeckerConfig(void) {
	Serial.println("Konfiguration speichern");
	DynamicJsonDocument json(1024);
	json["Wecker1_Stunden"] = Wecker1_Stunden;
   	json["Wecker1_Minuten"] = Wecker1_Minuten;
	json["Wecker1_Aktiv"] = Wecker1_Aktiv;  

    json["Wecker2_Stunden"] = Wecker2_Stunden;
   	json["Wecker2_Minuten"] = Wecker2_Minuten;
	json["Wecker2_Aktiv"] = Wecker2_Aktiv;  

    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
    	Serial.println("failed to open config file for writing");
 	} else {
		buzzer.On();
		delay(10);
		buzzer.Off();
	}
	serializeJson(json, Serial);
    serializeJson(json, configFile);
   	configFile.close();
}

// -----------------------------------------------------------------------------------
// init Zeit
// -----------------------------------------------------------------------------------
bool initTime(void) {
	bool result = false;

	time(&actualTime);					 // read the current time
	localtime_r(&actualTime, &timeinfo); // update the structure tm with the current time

	return result;
}

// -----------------------------------------------------------------------------------
// init timer irq
// -----------------------------------------------------------------------------------
void initIrq(void) {
	Serial.println(TraceTime() + "Konfigration der Interrupts");
	noInterrupts(); // Interrupts sperren

	timer0_isr_init();
	timer0_attachInterrupt(irqTimer0);
	timer0_write(ESP.getCycleCount() + 80000000L); // 80MHz == 1s

	interrupts(); 	// Interrups freigeben
}

// -----------------------------------------------------------------------------------
// hohle die Wetterdaten von openweathermap
// -----------------------------------------------------------------------------------
GBL_STATUS getWeatherDataState(void) {
	static uint16_t u16State = 0;
	static uint16_t u16StateOld = 1;
	static unsigned long AktZeit = 0;
	GBL_STATUS result = GBL_STATUS::WAIT;

	if (u16State != u16StateOld) {
		Serial.println(TraceTime() + "WD Status " + String(u16State));
		u16StateOld = u16State;
	}

	switch (u16State) {
		case 0:
			if (getWeather() != GBL_STATUS::READY) {
				result = GBL_STATUS::ERROR;
			} else {
				AktZeit = millis();
				result = GBL_STATUS::RUN;
				u16State = 10;
			}
			break;
		case 10:
			if (client.available()) {
				getWeatherData();
				result = GBL_STATUS::READY;
				u16State = 0;
			} else if (millis() > (AktZeit + 5000)) { 		// timeout
				result = GBL_STATUS::ERROR;
				u16State = 0;
			} else {
				result = GBL_STATUS::RUN;
			}
			break;
		default:
			u16State = 0;
			break;
	}
	return result;
}

GBL_STATUS getWeather(void) {
	GBL_STATUS result = GBL_STATUS::WAIT;

	Serial.println(TraceTime() + "getWeather");

	// /data/2.5/weather?id=2920512&appid=123456789abcdefg.....=json&units=metric
	// api.openweathermap.org/data/2.5/weather?id={city id}&appid={your api key}
	client.stop();
	Serial.print(TraceTime() + "Connection to ");
	Serial.println(Server);

	// Wenn Verbindung, dann Serverdaten abrufen
	if (client.connect(Server, 80)) {
		Serial.println(TraceTime() + "Connected to server");
		// HTTP Request starten
		String WebAsk = "GET /data/2.5/weather?id=" + CityId + "&appid=" + ApiKey;
		WebAsk = WebAsk + "&lang=de&mode=json&units=metric";
		Serial.println(TraceTime() + WebAsk);
		client.println(WebAsk);
		client.println("Host: api.openweathermap.org");
		client.println("User-Agent: ESP8266WiFi/1.1");
		client.println("Connection: close");
		client.println();
		result = GBL_STATUS::READY;
	} else {
		Serial.println(TraceTime() + "Unable to connect");
		result = GBL_STATUS::ERROR;
	}
	return result;
}

void getWeatherData(void) {
	Serial.println(TraceTime() + "getWeatherData");
	String WetterDaten = "";
	WetterDaten = WetterDaten + client.readString();
	Serial.print(TraceTime() + "Rohdaten:");
	Serial.println(WetterDaten);

	String Icon = WetterDatenAusJson(WetterDaten);
}

String WetterDatenAusJson(String WetterDaten) {
	Serial.println(TraceTime() + "WetterDatenAusJson");
	DeserializationError error = deserializeJson(doc, WetterDaten);

	if (error) {
		Serial.print(TraceTime() + "deserializeJson failed - ");
		Serial.println(error.c_str());
		return (String) "error";
	}

	// Daten vom JSON-Tree abrufen
	tempNow = doc["main"]["temp"];
	tempMin = doc["main"]["temp_min"];
	tempMax = doc["main"]["temp_max"];
	humidityNow = doc["main"]["humidity"];
	pressureNow = doc["main"]["pressure"];
	const char *city = doc["name"];
	const char *weather = doc["weather"][0]["description"];

	cityData = (char *)city;
	weatherNowData = (char *)weather;

	Serial.println("----------------------------------------------");
	Serial.print("Stadt        : ");
	Serial.println(city);
	Serial.print("Aktuelle Temp: ");
	Serial.print(tempNow);
	Serial.println("°C");
	Serial.print("Temp (Min)   : ");
	Serial.print(tempMin);
	Serial.println("°C");
	Serial.print("Temp (Max)   : ");
	Serial.print(tempMax);
	Serial.println("°C");
	Serial.print("Feuchtigkeit : ");
	Serial.print(humidityNow);
	Serial.println("%");
	Serial.print("Wetter       : ");
	Serial.println(weather);
	Serial.println("----------------------------------------------");

	Serial.print("memory used : ");
	Serial.println(doc.memoryUsage());

	return doc["weather"][0]["icon"];
}

// -----------------------------------------------------------------------------------
// callback Schalter 1 und 2
// -----------------------------------------------------------------------------------
ICACHE_RAM_ATTR void irqSw01(void) {
	if (ParamSw01.status) {
		led.On();
		buzzer.On();
	} else {
		buzzer.Off();
	}
}

ICACHE_RAM_ATTR void irqSw02(void) {
	led.Off();
}

// -----------------------------------------------------------------------------------
// callback Schalter 1 und 2
// -----------------------------------------------------------------------------------
String TraceTime(void) {
	char cTimeStampStr[60];

	// calc time (1 tick at u32TimerTick ==> T2_BASE)
	uint32_t u32TimerTick = millis();
	uint16_t u16ms = (uint16_t)((u32TimerTick) % 1000);
	uint16_t u16s = (uint16_t)((60 + (u32TimerTick / 1000)) % 60);
	uint16_t u16m = (uint16_t)((60 + (u32TimerTick / 60000)) % 60);
	uint16_t u16h = (uint16_t)(u32TimerTick / 3600000);
	sprintf(cTimeStampStr, "> %02u:%02u:%02u.%03u - ", u16h, u16m, u16s, u16ms);
	return String(cTimeStampStr);
}