// -----------------------------------------------------------------------------------
// Autor : Peter Schneider
// Datum : 23.01.2021 - change to VSCode and PlatformIO
//
// D1_mini with TFT 2.2", 320 x 240, SPI
// -----------------------------------------------------------------------------------
// pinning
// -----------------------------------------------------------------------------------
// VCC        - 3V3
// GND        - GND
// CS         -  D1   - GPIO-05
// RESET      - RST
// D/C        -  D4   - GPIO-02
// SDI / MOSI -  D7   - GPIO-13
// SCK        -  D5   - GPIO-14
// LED        -  D3   - GPIO-00 TFT backligth with PWM
// SDO / MISO -  D6   - GPIO-12 there is a seccond wireing with switch 1
// -----------------------------------------------------------------------------------
// LDR        -  A0   - ADC0    measure brigthness to change TFT backligth 
// Buzzer     -  D0	  - GPIO-16
// Switch 1   -  D6	  - GPIO-12
// Switch 2   -  D8   - GPIO-15
// LED        -  D2   - GPIO-04
// -----------------------------------------------------------------------------------
// to convert fonts to *.h : http://oleddisplay.squix.ch/#/home
// to convert images to *.h : http://rinkydinkelectronics.com/t_imageconverter565.php?msclkid=b9c2d4cdbd8811ec8ed10cdb25a780d0
// -----------------------------------------------------------------------------------
#include "Arduino.h"

#include <string.h>
#include <time.h>
#include <LittleFS.h>
#include <SPI.h>
#include <TFT_eSPI.h> 		
#include <TimeLib.h>
#include <ArduinoJson.h>

#include "font.h"
#include "Free_Fonts.h"
#include "TFT22-Uhr.h"

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
// config.h include the definition of API_KEY and CITY_KEY
// #define API_KEY "1234567890abcdefghijklmnopqrst..."
// #define CITY_KEY "1234..."
// ---------------------------------------------------------------------------------------------------
#include "config.h"   
// ---------------------------------------------------------------------------------------------------

#define MY_TZ "CET-1CEST,M3.5.0,M10.5.0/3"			// timezone with summertime and wintertime 
#define MY_NTP_SERVER "europe.pool.ntp.org" 		// used ntp timeserver

time_t actualTime;
struct tm timeinfo;

TFT_eSPI tft = TFT_eSPI();

// use openweather setup
String ApiKey = API_KEY;
String CityId = CITY_KEY; 
const char Server[] = "api.openweathermap.org";
StaticJsonDocument<2000> doc; 						 // make JSON doc
String strIcon;

const char *WeekDay[7] = {"So. ", "Mo. ", "Di. ", "Mi. ", "Do. ", "Fr. ", "Sa. "};

const uint16_t hSpace = 8;							 // space
const uint16_t yTop = 0;							 // start upper area
const uint16_t hTop = 40;							 // higth of upper area
const uint16_t yMiddle = hTop + hSpace;				 // start middle area
const uint16_t hMiddle = yMiddle + 95;				 // higth of middle area
const uint16_t yBottom = yMiddle + hMiddle + hSpace; // start lower area
const uint16_t hBottom = 40;						 // higth of lower area

uint16_t tftWidth;
uint16_t tftHeight;

WiFiManager wm;
WiFiClient client;

clOut led;
clOut buzzer;

// definition of switch 1
stInput ParamSw01 = {SW_01, CHANGE, 40, 2000, irqSw01, POLARITY::POS};
clIn sw01; 			

// definition of switch 2
stInput ParamSw02 = {SW_02, CHANGE, 40, 2000, irqSw02, POLARITY::POS};
clIn sw02;

char cVersion[] = {"02.00"};
char cDatum[]   = __DATE__;

// definition of wake up times
stWeckZeit stWz[MAX_WECKER] = {
 	{WOCHEN_TAG::MO, 5, 55},
	{WOCHEN_TAG::DI, 5, 55},
};

// definition of arlam clocks
std::array<clWecken, MAX_WECKER> Wecker = {
    clWecken(&timeinfo, &buzzer, &sw02, &stWz[0]),
    clWecken(&timeinfo, &buzzer, &sw02, &stWz[1]),
};

typedef struct {
	char strStunden[3];
	char strMinuten[3];
	char strTage[2];  
	char strAktiv[2];
} weck_daten_t;

weck_daten_t WeckerDaten[MAX_WECKER] = {
	{"00", "00", "0", " "},
	{"00", "00", "0", " "}
};

bool shouldSaveConfig = false;

menue_t hmMenue[5] = { 
//   function                 menue string             last item
	{runMainMenue,    String("Uhrzeit / Weckzeiten"),  false},
	{runWakeUpTime_1, String("Weckzeit 1 einstellen"), false},
	{runWakeUpTime_2, String("Weckzeit 2 einstellen"), false},
	{runState,        String("Statusanzeige"),         false},
	{runDeleteFile,   String("Delete Konfigiration"),   true}
};

// menue control
clMenue HMenue(&sw01, hmMenue, showState);

int pwmValue;

// define data from JSON-tree 
char *cityData;
float tempNow = 0.0;
float tempMin = 0.0;
float tempMax = 0.0;
float humidityNow = 0.0;
float pressureNow = 0.0;
char *weatherNowData;

bool bGetWeather = true;

// ---------------------------------------------------------------------------------------------------
// setup
// ---------------------------------------------------------------------------------------------------
void setup() {
	Serial.begin(115200);

	initDisplay();

	initGpio();
	
	initNetwork();

	tftBrigthnees();
	showFrame();
	
	configTime(MY_TZ, MY_NTP_SERVER);

	initFs();	// read konfig data of clock
	
	Serial.println();
	Serial.println("--------------------------------------");
	Serial.println("- TFT2.2 clock spi                    -");
	Serial.println("--------------------------------------");
	Serial.println(String("version    - ") + String(cVersion));
	Serial.println(String("build date - ") + String(cDatum));

	initIrq();

	showWeatherIcon(bild_44);
	showTemperature(tempNow);
}

// ---------------------------------------------------------------------------------------------------
// loop
// ---------------------------------------------------------------------------------------------------
void loop(void) {
	sw01.runState();
	sw02.runState();

	led.SwPort(sw01.Status());				// switch LED on with swith 1

	time(&actualTime);					 	// get actual time
	localtime_r(&actualTime, &timeinfo); 	// write actual time to timeinfo 

	clWecken::Check();						// check wake up time

	showTime(timeinfo);	 					
	showDateAndTime(timeinfo); 				
	tftBrigthnees();
	showWakeUpTime();
	
	HMenue.Verwaltung();					// run menue
	
	if (bGetWeather) {
		getActualWeather();	
		bGetWeather = false;
	}			   	
}

// ---------------------------------------------------------------------------------------------------
// show state line
// ---------------------------------------------------------------------------------------------------
void showLabel(void) {
	String strText = WiFi.SSID() + String(" - ") + WiFi.localIP().toString();
	showState(strText);
}

void showVersion(void) {
	String strText = String("Ver. ") + String(cVersion) + String(" - ") + String(cDatum); 
	showState(strText);
}

void showWeatherString(void) {
	//String strText = String(tempNow) + String(char(247)) + String("C, ") + String(weatherNowData);
	//showState(strText);

	showTemperature(tempNow);

	if (strIcon == String("01d")) {
		showWeatherIcon (bild_01d);
	} else if (strIcon == String("01n")) {
		showWeatherIcon (bild_01n);
	} else if (strIcon == String("02d")) {
		showWeatherIcon(bild_02d);
	} else if (strIcon == String("02n")) {
		showWeatherIcon(bild_02n);
	} else if (strIcon == String("03d")) {
		showWeatherIcon(bild_03d);
	} else if (strIcon == String("03n")) {
		showWeatherIcon(bild_03d);
	} else if (strIcon == String("04d")) {
		showWeatherIcon(bild_04d);
	} else if (strIcon == String("04n")) {
		showWeatherIcon(bild_04d);
	} else if (strIcon == String("09d")) {
		showWeatherIcon(bild_09d);
	} else if (strIcon == String("09n")) {
		showWeatherIcon(bild_09d);
	} else if (strIcon == String("10d")) {
		showWeatherIcon(bild_10d);
	} else if (strIcon == String("10n")) {
		showWeatherIcon(bild_10n);
	} else if (strIcon == String("13d")) {
		showWeatherIcon(bild_13d);
	} else if (strIcon == String("13n")) {
		showWeatherIcon(bild_13d);
	} else if (strIcon == String("50d")) {
		showWeatherIcon(bild_50d);
	} else if (strIcon == String("50n")) {
		showWeatherIcon(bild_50d);
	} else {
		showWeatherIcon(bild_44);
	}
}

void showWeatherIcon(const unsigned short* image) {
	uint16_t xpos = 6;
	uint16_t ypos = yMiddle + 3;

	tft.fillRect(xpos, ypos, 64, 64, TFT_BLACK);		// clear screen area
	tft.pushImage(xpos, ypos, 64, 64, image);
}

void showTemperature(float fTemp) {
	static String strTextOld = " ";
	String strText = String(fTemp,1) + String(char(247)) + String("C");

	tft.setTextSize(1);	
	//tft.setFreeFont(&Arimo_Regular_12);

	// clear actual string
	tft.setTextColor(TFT_BLACK, TFT_BLACK);
	tft.drawCentreString(strTextOld, 6 + 32, yMiddle + (hSpace + 64), 1);	

	// write new string
	tft.setTextColor(TFT_YELLOW, TFT_BLACK);
	tft.drawCentreString(strText, 6 + 32, yMiddle + (5 + 64), 1);

	//tft.setFreeFont(NULL);
	strTextOld = strText;	
}

void showState(String strData) {
	static String strOld = " ";

	tft.setTextSize(2);	

	// clear actual string
	tft.setTextColor(TFT_BLACK, TFT_BLACK);
	tft.drawCentreString(strOld, tftWidth / 2, yBottom + 12, 1);

	// draw new string
	tft.setTextColor(TFT_YELLOW, TFT_BLACK);
	tft.drawCentreString(strData, tftWidth / 2, yBottom + 12, 1);
	
	strOld = strData;
}

// ---------------------------------------------------------------------------------------------------
// menue functions
// ---------------------------------------------------------------------------------------------------
bool runMainMenue(void) {
	return clWecken::enableWakeUpTime(&sw02);
} 

bool changeWakeUpTime(uint16_t u16Nr) {
	static uint16_t u16Status = 0;
	static uint16_t u16StatusOld = 1;
		
	bool bResult = false;

	if (u16Status != u16StatusOld) {
		Serial.println(TraceTime() + String("changeWakeUpTime - ") + String(u16Status));
		u16StatusOld = u16Status;
	}

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
			if (u16Nr < MAX_WECKER) {	
				if (Wecker[u16Nr].stelleWeckzeit()) {
					Wecker[u16Nr].getWeckStunde().toCharArray(WeckerDaten[u16Nr].strStunden, 3);   
					Wecker[u16Nr].getWeckMinute().toCharArray(WeckerDaten[u16Nr].strMinuten, 3);   
					Wecker[u16Nr].getWeckTage().toCharArray(WeckerDaten[u16Nr].strTage, 2);   
					saveWeckerConfig();
					bResult = true;
					u16Status = 0;
				} 
			}
			break;
		default:
			u16Status = 0;
			break;	
	}
	return bResult;
}

bool runWakeUpTime_1(void) {
	return changeWakeUpTime(0);
}
bool runWakeUpTime_2(void) {
	return changeWakeUpTime(1);
}

bool runState(void) {
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
			showLabel();
			u32Timer = millis();
			u16Status = 20;
			break;
		case 20:
			if (millis() > (u32Timer + 4000)) {
				u16Status = 30;
			}
			break;
		case 30:
			showVersion();	
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
				showState(String("Konfig. deleted"));
			} else {
				showState(String("Fehler bei delete"));
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

void showFrame(void) {
	tft.fillScreen(TFT_BLACK);
	tft.drawRoundRect(1, yTop, tftWidth - 1, hTop, 10, TFT_BLUE);
	tft.drawRoundRect(1, yMiddle, tftWidth - 1, hMiddle, 10, TFT_BLUE);
	tft.drawRoundRect(1, yBottom, tftWidth - 1, hBottom, 10, TFT_BLUE);
}

void showDateAndTime(struct tm actTimeinfo) {
	char str[30];
	static String strZeitOld = " ";
	String strZeit;

	strZeit = String(WeekDay[actTimeinfo.tm_wday]);
	sprintf(str, "%02u.%02u.%4u - ", actTimeinfo.tm_mday, actTimeinfo.tm_mon + 1, 1900 + actTimeinfo.tm_year);
	strZeit += String(str);
	sprintf(str, "%02u:%02u:%02u", actTimeinfo.tm_hour, actTimeinfo.tm_min, actTimeinfo.tm_sec);
	strZeit += String(str);

	// write actual time if time changed
	if (strZeitOld != strZeit) {
		tft.setTextColor(TFT_YELLOW, TFT_BLACK);
		tft.drawCentreString(strZeit, tftWidth / 2, 12,1);
		strZeitOld = strZeit;
	}
}

void showWakeUpTime(void) {
	static String oldString1 = " ";
	static String oldString2 = " ";

	String Str1 = Wecker[0].getTimeString();
	String Str2 = Wecker[1].getTimeString();

	tft.setTextSize(2);	
	tft.setFreeFont(NULL);
	tft.setTextColor(TFT_YELLOW, TFT_BLACK);
	
	if (oldString1 != Str1) {
		tft.drawString(Str1, 20, 145, 1);
		oldString1 = Str1;
	}

	if (oldString2 != Str2) {
		tft.drawString(Str2, 20, 165, 1);
		oldString2 = Str2;
	}
}

// -----------------------------------------------------------------------------------------------------
// show time at the middel of the display
// -----------------------------------------------------------------------------------------------------
// The display is only updated when the seconds are at 0,so a new minute starts. 
// Since the first thing to do is completely delete the old time string.
// This create a short flicker of the time string.
// -----------------------------------------------------------------------------------------------------
void showTime(struct tm actTimeinfo) {
	static uint16_t u16State = 0;
	char str[8];
	String strZeit;
	static String strZeitOld = " ";
	String strInfo = "wait for NTP";

	switch (u16State)  	{
		case 0: 	// init time if ntp call is ready for the first time
			tft.setFreeFont(NULL);
			tft.setTextSize(2);	
			tft.setTextColor(TFT_YELLOW, TFT_BLACK);
			tft.drawRightString(strInfo, tftWidth - 10, 90, 1);
			u16State = 5;
			break;
		case 5: 	// check for a valide year > 2000 (1900 + 100) 
			if (actTimeinfo.tm_year > 100) {
				tft.setTextColor(TFT_BLACK, TFT_BLACK);
				tft.drawRightString(strInfo, tftWidth - 10, 90, 1);
				u16State = 20;
			} else {
				u16State = 6;
			}
			break;
		case 6: 	// wait 100ms
			delay(100);
			u16State = 5;
			break;
		case 10: 	// wait for a new minute (seccond == 0)
			if (actTimeinfo.tm_sec == 0) {
				u16State = 20;
			}
			break;
		case 20: 	// show new time string
			// build new time string
			sprintf(str, "%u:%02u", actTimeinfo.tm_hour, actTimeinfo.tm_min);
			strZeit = String(str);
			
			tft.setTextSize(1);	
			tft.setFreeFont(&Arimo_Regular_95);
			
			// clear actual time
			tft.setTextColor(TFT_BLACK, TFT_BLACK);
			tft.drawRightString(strZeitOld, tftWidth - 10, (tftHeight / 2) - 65, 1);
			
			// write new time
			tft.setTextColor(TFT_YELLOW, TFT_BLACK);
			tft.drawRightString(strZeit, tftWidth - 10, (tftHeight / 2) - 65, 1);

			tft.setFreeFont(NULL);			
			strZeitOld = strZeit;
			u16State = 30;
			break;
		case 30: 	// wait for seccond != 0
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
// TFT backligth brightness, controlled by LDR 
// -------------------------------------------------------------------------------------------------
void tftBrigthnees(void) {
	// pwmValue = 4 * measured value - 68 -> linear relationship determined empirically
	pwmValue = (4 * analogRead(TFT_POTI)) - 68;		// analog value 0 - 1024

	if (pwmValue < 1) {pwmValue = 1;}				// set lower limit to 1
	if (pwmValue > 1024) {pwmValue = 1024;}			// set upper limit to 1024

	analogWrite(TFT_BACKLIGHT, pwmValue);
}

// -----------------------------------------------------------------------------------
// interrupt function - timer 0 - 1s
// -----------------------------------------------------------------------------------
IRAM_ATTR void irqTimer0(void) {
	//Serial.println(TraceTime() + "Timer IRQ");

	static uint32_t weatherTimer = 0;

	if (++weatherTimer >= 1800) {
		weatherTimer = 0;
		bGetWeather = true;
	}

	timer0_write(ESP.getCycleCount() + 80000000L); // for 80MHz this means 1 interrupt per seccond
}

// -----------------------------------------------------------------------------------
// init GPIO
// -----------------------------------------------------------------------------------
void initGpio(void) {
	sw01.Init(ParamSw01);
	sw02.Init(ParamSw02);

	// Summer
	buzzer.Init(BUZZER, POLARITY::NEG); // GIPO BUZZER low active
	buzzer.On();
	delay(10);
	buzzer.Off();

	// LED
	led.Init(LED, POLARITY::POS); 		// GPIO LED high active
	
	// init an switch on TFT backligth 
	analogWriteRange(1024);
	analogWriteFreq(5000);
	analogWrite(TFT_BACKLIGHT, 1024); 	// valid value 0 - 1024
}

// -----------------------------------------------------------------------------------
// init display
// -----------------------------------------------------------------------------------
void initDisplay(void) {
	tft.init();
	tft.setSwapBytes(true);				// used by push image function
	tft.setRotation(ROTATION_90);
	tft.fillScreen(TFT_BLACK);

	tft.setTextColor(TFT_WHITE, TFT_BLACK);

	tftWidth = tft.width();
	tftHeight = tft.height();
}

// -----------------------------------------------------------------------------------
// init network
// -----------------------------------------------------------------------------------
void initNetwork(void) {
	wm.setAPCallback(wifiCallback);
	wm.setSaveConfigCallback(saveConfigCallback);

	tft.setCursor(0, 30);
	tft.setTextSize(2);				
	tft.drawString("Netzwerverbindung starten", 10, 10, 1);

	if (wm.autoConnect("ESP_TFT_UHR")) {
		tft.drawString(".. Netzwerverbindung OK", 10, 40, 1);
		delay(2000);
	} else {
		tft.drawString(".. Netzwerkfehler !!", 10, 40, 1);
		tft.drawString(".. ESP startet neu !!", 10, 70, 1);
		delay(5000);
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
// init file system and read configuration
// -----------------------------------------------------------------------------------
void initFs(void) {
	Serial.println(".. mounting FS");

  	if (LittleFS.begin()) {

		//return;  

   		Serial.println(".. mounted file system");
    	if (LittleFS.exists("/config.json")) {
      		// file exists, reading and loading
      		Serial.println(".. reading config file");
      		File configFile = LittleFS.open("/config.json", "r");
      		if (configFile) {
        		Serial.println(".. opened config file");
        		size_t size = configFile.size();
        		
				// allocate a buffer to store contents of the file.
        		std::unique_ptr<char[]> buf(new char[size]);

        		configFile.readBytes(buf.get(), size);

		       	DynamicJsonDocument json(1024);
        		auto deserializeError = deserializeJson(json, buf.get());
        		serializeJson(json, Serial);
        		if (!deserializeError) {
          			Serial.println("\nparsed json");
					char strData[20];  
					for (int i = 0; i < MAX_WECKER; i++) {
						sprintf(strData, "WeckerStunden_%i" , i);
						strcpy(WeckerDaten[i].strStunden, json[strData]);
						
						sprintf(strData, "WeckerMinuten_%i" , i);
						strcpy(WeckerDaten[i].strMinuten, json[strData]);
						
						sprintf(strData, "WeckerTage_%i" , i);
						strcpy(WeckerDaten[i].strTage, json[strData]);
						
						sprintf(strData, "WeckerAktiv_%i" , i);
						strcpy(WeckerDaten[i].strAktiv, json[strData]); 

						String Data = (String)WeckerDaten[i].strStunden;
						stWz[i].u16Stunde = Data.toInt();  

						Data = (String)WeckerDaten[i].strMinuten;
						stWz[i].u16Minute = Data.toInt();

						Data = (String)WeckerDaten[i].strTage;
						stWz[i].Wochentag = (WOCHEN_TAG)Data.toInt();

						Wecker[i].setTime(&stWz[i]);
						if (WeckerDaten[i].strAktiv[0] == '*') {
							Serial.println((String)".. Wecker " + String(i) + (String)" ist aktiv");
							Wecker[i].Start();
						}	
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
}

// callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Konfiguration sollte gespeichert werden");
  shouldSaveConfig = true;
}

void saveWeckerConfig(void) {
	Serial.println("Konfiguration speichern");
	DynamicJsonDocument json(1024);
	char strData[20]; 
	
	for(int i = 0; i < MAX_WECKER; i++) {
		sprintf(strData, "WeckerStunden_%i" , i);
		json[strData] = WeckerDaten[i].strStunden;
		
		sprintf(strData, "WeckerMinuten_%i" , i);
		json[strData] = WeckerDaten[i].strMinuten;
		
		sprintf(strData, "WeckerTage_%i" , i);
		json[strData] = WeckerDaten[i].strTage;

		sprintf(strData, "WeckerAktiv_%i" , i);
		if (Wecker[i].getStatus()) {
			json[strData] = "*";  
		} else {
			json[strData] = " ";  
		}
	}
    
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
// init time
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
	noInterrupts(); // disable all interrupts 

	timer0_isr_init();
	timer0_attachInterrupt(irqTimer0);
	timer0_write(ESP.getCycleCount() + 80000000L); // 80MHz == 1s

	interrupts(); 	// enable all interrups
}

// -----------------------------------------------------------------------------------
// get weather data from internet - https:\\openweathermap.org
// -----------------------------------------------------------------------------------
String getActualWeather(void) {
	String strResult = "no data";
	String strUrl = "GET /data/2.5/weather?id=" + CityId + "&appid=" + ApiKey + "&lang=de&mode=json&units=metric";
	
	Serial.println(TraceTime() + "getActualWeather");

	if (client.connect(Server, 80)) {
		client.println(strUrl);
		strResult = client.readString();
	}

	Serial.println(TraceTime() + strResult);

	if (strResult != "no data") {
		strIcon = getJsonWeatherString(strResult);
		showWeatherString();
	}

	return strResult;
}

String getJsonWeatherString(String WetterDaten) {
	Serial.println(TraceTime() + "getJsonWeatherString");
	DeserializationError error = deserializeJson(doc, WetterDaten);

	if (error) {
		Serial.print(TraceTime() + "deserializeJson failed - ");
		Serial.println(error.c_str());
		return (String) "error";
	}

	// get data from JSON-tree 
	tempNow = doc["main"]["temp"];
	tempMin = doc["main"]["temp_min"];
	tempMax = doc["main"]["temp_max"];
	humidityNow = doc["main"]["humidity"];
	pressureNow = doc["main"]["pressure"];
	const char *city = doc["name"];
	const char *weather = doc["weather"][0]["description"];
	const char *icon = doc["weather"][0]["icon"];

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
	Serial.print("Icon Name    : ");
	Serial.println(icon);
	Serial.println("----------------------------------------------");

	Serial.print("memory used : ");
	Serial.println(doc.memoryUsage());

	return icon;
}

// -----------------------------------------------------------------------------------
// callback Schalter 1 und 2 - nur als Beispiel
// -----------------------------------------------------------------------------------
ICACHE_RAM_ATTR void irqSw01(void) {
	if (sw01.Status()) {
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