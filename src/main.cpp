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
// to convert fonts  to *.h : http://oleddisplay.squix.ch/#/home
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
#include <ArduinoOTA.h> 		// OTA Upload via ArduinoIDE

#include "font.h"
#include "TFT22-Uhr.h"

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
TFT_eSprite actTimeToShow    = TFT_eSprite(&tft); 	// sprite object actTimeToShow
TFT_eSprite actDateToShow    = TFT_eSprite(&tft);	// sprite to show actual date
TFT_eSprite actTimeSecToShow = TFT_eSprite(&tft);	// sprite to show actual time and sec.

// use openweather setup
String ApiKey = API_KEY;
String CityId = CITY_KEY; 
const char Server[] = "api.openweathermap.org";
String strIcon;

const char WeekDay[7][5] = {"So. ", "Mo. ", "Di. ", "Mi. ", "Do. ", "Fr. ", "Sa. "};

const GFXfont *DefaultFont = &Arimo_Regular_24;
const GFXfont *TimeFont = &Arimo_Regular_95;
const GFXfont *IconFont = &Arimo_Regular_12;

const uint16_t hSpace = 8;							 // space
const uint16_t yTop = 0;							 // start upper area
const uint16_t hTop = 40;							 // higth of upper area
const uint16_t yMiddle = hTop + hSpace;				 // start middle area
const uint16_t hMiddle = yMiddle + 95;				 // higth of middle area
const uint16_t yBottom = yMiddle + hMiddle + hSpace; // start lower area
const uint16_t hBottom = 40;						 // higth of lower area

const uint16_t xPosWeatherNow = 6;
const uint16_t yPosWeatherNow = yMiddle + 5;

uint16_t tftWidth;
uint16_t tftHeight;

WiFiManager wifiManager;
WiFiClient wifiClient;

clOut led;
clOut buzzer;

// definition of switch 1
stInput ParamSw01 = {SW_01, CHANGE, 40, 2000, irqSw01, POLARITY::POS, false};
clIn sw01; 			

// definition of switch 2
stInput ParamSw02 = {SW_02, CHANGE, 40, 2000, irqSw02, POLARITY::POS, false};
clIn sw02;

char cVersion[] = "02.00";
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

menue_t hmMenue[6] = { 
//   function                   menue string             last item
	{runMainMenue,    	String("Uhrzeit / Weckzeiten"),  false},
	{runWakeUpTime_1, 	String("Weckzeit 1 einstellen"), false},
	{runWakeUpTime_2, 	String("Weckzeit 2 einstellen"), false},
	{runWeatherForcast, String("Wettervorschau"),        false},
	{runState,        	String("Statusanzeige"),         false},
	{runDeleteFile,   	String("Delete Konfiguration"),   true}
};

// menue control
clMenue HMenue(&sw01, hmMenue, showState);

int pwmValue;

// define data from JSON-tree 
float tempToday = 0.0;
float tempMinToday = 0.0;
float tempMaxToday = 0.0;
float humidityToday = 0.0;
float pressureToday = 0.0;

String strIconToday;
String strWeatherToday;
String strCityNameToday;

// define forcast data from JSON-tree
#define FORECAST	4 
float tempDayForecast[FORECAST];
float tempNigthForecast[FORECAST];
float tempMinForecast[FORECAST];
float tempMaxForecast[FORECAST];
float humidityForecast[FORECAST];
float pressureForecast[FORECAST];

String strWeatherForecast[FORECAST];
String strIconForecast[FORECAST];
String strCityNameForecast;

bool bGetWeather = false;

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
	showWeatherIcon(bild_44, xPosWeatherNow, yPosWeatherNow);

	// config OTA 
 	ArduinoOTA.onStart([]() {  
	 	tft.fillScreen(TFT_BLACK);
		tft.setFreeFont(DefaultFont);
		tft.setTextColor(TFT_WHITE, TFT_BLACK);
		tft.setCursor(0,30);
		tft.println(".. Start Update");
	});
 	ArduinoOTA.onEnd([]() {  
		tft.println(" ");
		tft.println(".. Restart System");
		delay(2000);
  	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		static uint16_t u16FirstCall = true;
		static uint16_t u16Count = 0;
		if (u16FirstCall) {
			tft.print(".. Progress: ");
			u16FirstCall = false;
		} else {
			if (u16Count++ == 25) {
				tft.print(".");
				u16Count = 0;
			}
			if (total == progress) {
				tft.println(" ");
				tft.print(".. Update ready");
			}
		}
	});
	ArduinoOTA.begin(); 	
}

// ---------------------------------------------------------------------------------------------------
// loop
// ---------------------------------------------------------------------------------------------------
void loop(void) {
	//String strText = String("T:") + String(tempToday, 1) + String(char(247)) + String("C, ") + String("H:") + String(humidityToday, 1) + String("%");
	String strText = String("T:") + String(tempToday, 1) + String("'C, ") + String("H:") + String(humidityToday, 1) + String("%");
	static String strTextOld = " ";

	ArduinoOTA.handle(); 					// OTA Upload via ArduinoIDE
	
	sw01.runState();
	sw02.runState();

	led.SwPort(sw01.Status());				// switch LED on with swith 1 - only for test

	time(&actualTime);					 	// get actual time
	localtime_r(&actualTime, &timeinfo); 	// write actual time to timeinfo 

	clWecken::Check();						// check wake up time

	showDateAndTime(timeinfo); 				
	tftBrigthnees();
	showWakeUpTime(false);
	
	HMenue.Verwaltung();					// run menue
	
	if (HMenue.getAktualMenue() != 0) {
		strTextOld = " ";
	}

	// show clock and weather only at menue piont 0
	switch (HMenue.getAktualMenue()) {
		case 0:
			if (strTextOld != strText) {
				strTextOld = strText;
				showState(strText);
			}
		case 1:
		case 2:
		case 4:
		case 5:
			showTime(timeinfo, false);
			if (bGetWeather) {
				getActualWeather();	
				showWeather(strIconToday, xPosWeatherNow, yPosWeatherNow);
				bGetWeather = false;
			}	
			break;
		case 3:
			break;
	}		   	
}

void showLabel(void) {
	String strText = WiFi.SSID() + String(" - ") + WiFi.localIP().toString();
	showState(strText);
}

void showVersion(void) {
	String strText = String("Ver. ") + String(cVersion) + String(" - ") + String(cDatum); 
	showState(strText);
}

void showWeather(String _strIcon, uint16_t _xpos, uint16_t _ypos) {
	if      (_strIcon == String("01d")) {showWeatherIcon(bild_01d, _xpos, _ypos);}
	else if (_strIcon == String("01n")) {showWeatherIcon(bild_01n, _xpos, _ypos);}
	else if (_strIcon == String("02d")) {showWeatherIcon(bild_02d, _xpos, _ypos);}
	else if (_strIcon == String("02n")) {showWeatherIcon(bild_02n, _xpos, _ypos);}
	else if (_strIcon == String("03d")) {showWeatherIcon(bild_03d, _xpos, _ypos);}
	else if (_strIcon == String("03n")) {showWeatherIcon(bild_03d, _xpos, _ypos);}
	else if (_strIcon == String("04d")) {showWeatherIcon(bild_04d, _xpos, _ypos);}
	else if (_strIcon == String("04n")) {showWeatherIcon(bild_04d, _xpos, _ypos);}
	else if (_strIcon == String("09d")) {showWeatherIcon(bild_09d, _xpos, _ypos);}
	else if (_strIcon == String("09n")) {showWeatherIcon(bild_09d, _xpos, _ypos);}
	else if (_strIcon == String("10d")) {showWeatherIcon(bild_10d, _xpos, _ypos);}
	else if (_strIcon == String("10n")) {showWeatherIcon(bild_10n, _xpos, _ypos);}
	else if (_strIcon == String("13d")) {showWeatherIcon(bild_13d, _xpos, _ypos);}
	else if (_strIcon == String("13n")) {showWeatherIcon(bild_13d, _xpos, _ypos);}
	else if (_strIcon == String("50d")) {showWeatherIcon(bild_50d, _xpos, _ypos);}
	else if (_strIcon == String("50n")) {showWeatherIcon(bild_50d, _xpos, _ypos);}
	else {showWeatherIcon(bild_44, _xpos, _ypos);}
}

void showWeatherIcon(const unsigned short* _image, uint16_t _xpos, uint16_t _ypos) {
	static String strTimeOld = " ";
	String strTime;
	uint16_t u16Width = 64;
	uint16_t u16Higth = 64;
	
	tft.fillRect(_xpos, _ypos, u16Width, u16Higth, TFT_BLACK);	
	tft.pushImage(_xpos, _ypos, 64, 64, _image);
	
	//tft.setFreeFont(IconFont);
	//tft.setTextColor(TFT_BLACK);
	//tft.drawCentreString(strTimeOld, _xpos + 32, _ypos + 64 + 2, 1);

	//sprintf(str, "%02u:%02u", timeinfo.tm_hour, timeinfo.tm_min);
	//strTime = String(str);
	//tft.setTextColor(TFT_YELLOW);
	//tft.drawCentreString(strTime, _xpos + 32, _ypos + 64 + 2, 1);

	strTimeOld = strTime;
}

void showState(String _strData) {
	static String strOld = " ";

	tft.setFreeFont(DefaultFont);
	tft.setTextSize(1);	

	// clear actual string
	tft.setTextColor(TFT_BLACK, TFT_BLACK);
	tft.drawCentreString(strOld, tftWidth / 2, yBottom + 9, 1);

	// draw new string
	tft.setTextColor(TFT_YELLOW, TFT_BLACK);
	tft.drawCentreString(_strData, tftWidth / 2, yBottom + 9, 1);
	tft.setFreeFont(NULL);

	strOld = _strData;
}

// ---------------------------------------------------------------------------------------------------
// menue functions
// ---------------------------------------------------------------------------------------------------
bool runMainMenue(void) {
	return clWecken::enableWakeUpTime(&sw02);
} 

bool changeWakeUpTime(uint16_t _u16Nr) {
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
			if (_u16Nr < MAX_WECKER) {	
				if (Wecker[_u16Nr].stelleWeckzeit()) {
					Wecker[_u16Nr].getWeckStunde().toCharArray(WeckerDaten[_u16Nr].strStunden, 3);   
					Wecker[_u16Nr].getWeckMinute().toCharArray(WeckerDaten[_u16Nr].strMinuten, 3);   
					Wecker[_u16Nr].getWeckTage().toCharArray(WeckerDaten[_u16Nr].strTage, 2);   
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
				showState(String("config. deleted"));
			} else {
				showState(String("delete error"));
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

bool runWeatherForcast (void) {
	static uint16_t u16Status = 0;
	static uint32_t u32Timer = 0;
	bool bResult = false;
	
	switch (u16Status) {
		case 0:
			bResult = true;
			if (sw02.Status()) {
				Serial.printf("get Weather forcast");
				showState("warte auf Wetterdaten");
				u16Status = 10;
			}
			break;
		case 10:
			getWeatherForcast();
			showState("Wettervorschau");
			u16Status = 20;
			break;
		case 20:	
			if (!sw02.Status()) {
				u32Timer = millis();
				u16Status = 30;
			}
			break;
		case 30:
			if (sw02.Status()) {
				u16Status = 40;
			} else if (millis() > (u32Timer + (15 * 60000))) {
				u16Status = 40;
			}
			break;
		case 40:
			if (!sw02.Status()) {
				tft.fillRect(5, yMiddle + 5, tftWidth - 10, hMiddle - 10, TFT_BLACK);
				bGetWeather = true;
				showWeather(strIconToday, xPosWeatherNow, yPosWeatherNow);
				showWakeUpTime(true);
				showTime(timeinfo, true);
				u16Status = 0;
			}
			break;
		default:
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

void showDateAndTime(struct tm _actTimeinfo) {
	uint16_t spriteHigth = 30;
	
	// date
	uint16_t spriteWidth1 = (tftWidth / 2) + 16;
	uint16_t spriteXPos1  = 6;	
	uint16_t spriteYPos1  = 4;

	// time
	uint16_t spriteWidth2 = (tftWidth / 2) - 28;
	uint16_t spriteXPos2  = (tftWidth / 2) + 22;	
	uint16_t spriteYPos2  = 4;

	char str[20];
	static String strTimeOld = " ";
	String strTime;
	
	static String strDateOld = " ";
	String strDate;

	sprintf(str, "%02u:%02u:%02u", _actTimeinfo.tm_hour, _actTimeinfo.tm_min, _actTimeinfo.tm_sec);
	strTime = String(str);
	
	// write actual time if time changed
	if (strTimeOld != strTime) {
		actTimeSecToShow.setColorDepth(8);
		actTimeSecToShow.createSprite(spriteWidth2, spriteHigth);
		actTimeSecToShow.fillSprite(TFT_BLACK);
		
		actTimeSecToShow.setFreeFont(DefaultFont);
		actTimeSecToShow.setTextSize(1);	
		actTimeSecToShow.setTextColor(TFT_YELLOW);
		actTimeSecToShow.drawRightString(strTime, spriteWidth2, (spriteHigth / 2) - 10, 1);
		
		actTimeSecToShow.pushSprite(spriteXPos2, spriteYPos2);	// show sprite at screen
		strTimeOld == strTime;
	}

	strDate = String(WeekDay[_actTimeinfo.tm_wday]);
	sprintf(str, "%02u.%02u.%4u", _actTimeinfo.tm_mday, _actTimeinfo.tm_mon + 1, 1900 + _actTimeinfo.tm_year);
	strDate += String(str);
	
	// write actual date if time changed
	if (strDateOld != strDate) {
		actDateToShow.setColorDepth(8);
		actDateToShow.createSprite(spriteWidth1, spriteHigth);
		actDateToShow.fillSprite(TFT_BLACK);
	
		actDateToShow.setFreeFont(DefaultFont);
		actDateToShow.setTextSize(1);	
		actDateToShow.setTextColor(TFT_YELLOW);
	
		actDateToShow.drawString(strDate, 0, (spriteHigth / 2) - 10, 1);
		actDateToShow.pushSprite(spriteXPos1, spriteYPos1);
		strDateOld == strDate;
	}
}

void showWakeUpTime(bool _bForce) {
	static String oldString[MAX_WECKER];
	String strWakupTime[MAX_WECKER];
	String strName[MAX_WECKER];
	String strHour[MAX_WECKER];
	String strMinute[MAX_WECKER];
	String strDay[MAX_WECKER];
	
	tft.setFreeFont(DefaultFont);
	tft.setTextSize(1);	
	tft.setTextColor(TFT_YELLOW, TFT_BLACK);

	for (int i = 0; i < MAX_WECKER; i++) {
		uint16_t y = 140 + (i * 25); 
		strWakupTime[i] = Wecker[i].getTimeString(); 
		strName[i]   = strWakupTime[i].substring(0,6);
		strHour[i]   = strWakupTime[i].substring(6,8);
		strMinute[i] = strWakupTime[i].substring(9,11);
		strDay[i]    = strWakupTime[i].substring(14,strWakupTime[i].length());

		if ((oldString[i] != strWakupTime[i]) || _bForce) {
			Serial.println(TraceTime() + strWakupTime[i]);

			tft.drawString(strName[i], 10, y);
			if (strHour[i].substring(0,1) != " ") {
				tft.drawString(strHour[i], 90, y);
			} else {
				tft.fillRect(90, y, 30, 21, TFT_BLACK);
			}
			tft.drawString(":", 120, y);
			if (strMinute[i].substring(0,1) != " ") {
				tft.drawString(strMinute[i], 130, y);
			} else {
				tft.fillRect(130, y, 30, 21, TFT_BLACK);
			}
			tft.drawString(" : ", 158, y);
			if (strDay[i].substring(0,1) != " ") {
				tft.drawString(strDay[i], 180, y);
			} else {
				tft.fillRect(180, y, 320 - 190, 21, TFT_BLACK);
			}
			oldString[i] = strWakupTime[i];
		}	
	}
}

// -----------------------------------------------------------------------------------------------------
// show time at the middel of the display
// -----------------------------------------------------------------------------------------------------
// The display is only updated when the seconds are at 0, so a new minute starts. 
// First thing to do is completely delete the old time string by writing the string 
// with set the text color to the background color.
// Then we can write the new time string to the sprite object.
// At last the sprite object is shown at the display.
// -----------------------------------------------------------------------------------------------------
void showTime(struct tm _actTimeinfo, bool _bForce) {
	static uint16_t u16State = 0;
	static String strZeitOld = " ";

	uint16_t spriteWidth = tftWidth - 84;
	uint16_t spriteHigth = 85;
	uint16_t spriteYPos  = yMiddle + hSpace;
	uint16_t spriteXPos  = 76;	

	char str[8];
	String strZeit;
	String strInfo = "wait for NTP";

	switch (u16State)  	{
		case 0: 	// init time if ntp call is ready for the first time
			actTimeToShow.setColorDepth(8);
			actTimeToShow.createSprite(spriteWidth, spriteHigth);
			actTimeToShow.setFreeFont(DefaultFont);
			actTimeToShow.setTextSize(1);		
			actTimeToShow.fillSprite(TFT_BLACK);

			actTimeToShow.setTextColor(TFT_YELLOW);
			actTimeToShow.drawRightString(strInfo, spriteWidth - 10, (spriteHigth / 2) - 14, 1);
			actTimeToShow.pushSprite(spriteXPos, spriteYPos);			// show sprite at screen
			u16State = 5;
			break;
		case 5: 	// check for a valide year > 2000 (1900 + 100) 
			if (_actTimeinfo.tm_year > 100) {
				actTimeToShow.fillSprite(TFT_BLACK);
				bGetWeather = true;
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
			if ((_actTimeinfo.tm_sec == 0) || _bForce) {
				u16State = 20;
			}
			break;
		case 20: 	// show new time string
			// build new time string
			sprintf(str, "%u:%02u", _actTimeinfo.tm_hour, _actTimeinfo.tm_min);
			strZeit = String(str);
			
			actTimeToShow.setTextSize(1);	
			actTimeToShow.setFreeFont(TimeFont);
			
			// clear actual time
			actTimeToShow.setTextColor(TFT_BLACK);
			actTimeToShow.drawRightString(strZeitOld, spriteWidth, 0, 1);
			
			// write new time
			actTimeToShow.setTextColor(TFT_YELLOW);
			actTimeToShow.drawRightString(strZeit, spriteWidth, 0, 1);
			
			// show sprite at screen
			actTimeToShow.pushSprite(spriteXPos, spriteYPos);

			strZeitOld = strZeit;
			u16State = 30;
			break;
		case 30: 	// wait for seccond != 0
			if (_actTimeinfo.tm_sec != 0) {
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

	if (++weatherTimer >= 60 * 15) {
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
	wifiManager.setAPCallback(wifiCallback);
	wifiManager.setSaveConfigCallback(saveConfigCallback);

	tft.setCursor(0, 30);
	tft.setFreeFont(DefaultFont);
	tft.setTextSize(1);	
	tft.drawString(".. start WLan", 10, 10);

	if (wifiManager.autoConnect("ESP_TFT_UHR")) {
		tft.drawString(".. WLan connected", 10, 40);
		String strText = String(".. ") + WiFi.SSID() + String(" - ") + WiFi.localIP().toString();
		tft.drawString(strText, 10, 70);
		delay(4000);
	} else {
		tft.drawString(".. WLan error !!", 10, 40);
		tft.drawString(".. ESP Reset !!", 10, 70);
		delay(5000);
		while (true) {;}
	}
}

void wifiCallback(WiFiManager *_myWiFiManager) {
	tft.println(".. Konfig. Mode aktiv");
	tft.print("..");
	tft.println(WiFi.softAPIP());
	tft.println(String("..") + _myWiFiManager->getConfigPortalSSID());
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
// read data from web page
// -----------------------------------------------------------------------------------
String getJsonDataFromWeb (String _Server, String _Url) {
	uint16_t u16Tries = 0;

	while ((!wifiClient.connect(_Server, 80)) && (u16Tries < 5)) {
		u16Tries++;
		Serial.println(TraceTime() + String("connection error") + u16Tries);
		delay(100);
	}

	Serial.println(TraceTime() + String("connect to ") + Server);
	wifiClient.println(_Url);
	return wifiClient.readString();
}

// -----------------------------------------------------------------------------------
// get weather data from internet - https:\\openweathermap.org
// -----------------------------------------------------------------------------------
void getActualWeather(void) {
	String strUrl = "GET /data/2.5/weather?id=" + CityId + "&appid=" + ApiKey + "&lang=de&mode=json&units=metric";
	
	Serial.println(TraceTime() + "getActualWeather");
	Serial.println(TraceTime() + strUrl);
		
	decodeCurrentWeather(getJsonDataFromWeb(Server, strUrl));
}

void decodeCurrentWeather(String _WetterDaten) {
	DynamicJsonDocument jsonWeatherToday(900);

	Serial.println(TraceTime() + "decodeCurrentWeather");
	
	DeserializationError error = deserializeJson(jsonWeatherToday, _WetterDaten);

	if (error) {
		Serial.print(TraceTime() + "deserializeJson failed - ");
		Serial.println(error.c_str());
	} else {
		// get data from JSON-tree 
		tempToday     = jsonWeatherToday["main"]["temp"].as<float>();
		tempMinToday  = jsonWeatherToday["main"]["temp_min"].as<float>();
		tempMaxToday  = jsonWeatherToday["main"]["temp_max"].as<float>();
		humidityToday = jsonWeatherToday["main"]["humidity"].as<float>();
		pressureToday = jsonWeatherToday["main"]["pressure"].as<float>();

		strCityNameToday = jsonWeatherToday["name"].as<String>();
		strWeatherToday  = jsonWeatherToday["weather"][0]["description"].as<String>();
		strIconToday     = jsonWeatherToday["weather"][0]["icon"].as<String>();

		Serial.println("----------------------------------------------");
		Serial.print("Stadt        : ");
		Serial.println(strCityNameToday);
		Serial.print("Aktuelle Temp: ");
		Serial.print(tempToday);
		Serial.println("°C");
		Serial.print("Temp (Min)   : ");
		Serial.print(tempMinToday);
		Serial.println("°C");
		Serial.print("Temp (Max)   : ");
		Serial.print(tempMaxToday);
		Serial.println("°C");
		Serial.print("Feuchtigkeit : ");
		Serial.print(humidityToday);
		Serial.println("%");
		Serial.print("Wetter       : ");
		Serial.println(strWeatherToday);
		Serial.print("Icon Name    : ");
		Serial.println(strIconToday);
		Serial.println("----------------------------------------------");

		Serial.print("memory used : ");
		Serial.println(jsonWeatherToday.memoryUsage());
	}
}

void getWeatherForcast(void) {
	String strUrl = "GET /data/2.5/forecast/daily?id=" + CityId + "&appid=" + ApiKey + "&cnt=4&lang=de&mode=json&units=metric";
	
	Serial.println(TraceTime() + "getWeatherForcast");
	Serial.println(TraceTime() + strUrl);

	decodeWeatherForcast(getJsonDataFromWeb(Server, strUrl));
}

void decodeWeatherForcast(String _WetterDaten) {
	DynamicJsonDocument jsonWeatherForecast(2500);

	uint16_t u16Count = 0;
	time_t ForecastTime;
	char strData[30];
	char cDay[FORECAST][5];

	Serial.println(TraceTime() + "decodeWeatherForcast");
	
	DeserializationError error = deserializeJson(jsonWeatherForecast, _WetterDaten);

	if (error) {
		Serial.print(TraceTime() + "deserializeJson failed - ");
		Serial.println(error.c_str());
		return;
	} else {
		// get data from JSON-tree 
		u16Count = jsonWeatherForecast["cnt"];
		Serial.println(String("Wettervorhersage für ") + u16Count + String (" Tage"));
		
		if (u16Count > FORECAST) {
			u16Count = FORECAST;
		}
	
		for (int i=0; i<u16Count; i++) {
			yield();
			strCityNameForecast  = jsonWeatherForecast["city"]["name"].as<String>();
			ForecastTime         = jsonWeatherForecast["list"][i]["dt"].as<long int>();
			tempDayForecast[i]   = jsonWeatherForecast["list"][i]["temp"]["day"].as<float>();
			tempNigthForecast[i] = jsonWeatherForecast["list"][i]["temp"]["nigth"].as<float>(); 
			tempMinForecast[i]   = jsonWeatherForecast["list"][i]["temp"]["min"].as<float>();
			tempMaxForecast[i]   = jsonWeatherForecast["list"][i]["temp"]["max"].as<float>();
			humidityForecast[i]  = jsonWeatherForecast["list"][i]["humidity"].as<float>();
			pressureForecast[i]  = jsonWeatherForecast["list"][i]["pressure"].as<float>();

			strWeatherForecast[i] = jsonWeatherForecast["list"][i]["weather"][0]["description"].as<String>();
			strIconForecast[i] = jsonWeatherForecast["list"][i]["weather"][0]["icon"].as<String>();

			strcpy(&cDay[i][0], WeekDay[weekday(ForecastTime) - 1]);

			Serial.println("----------------------------------------------");
			sprintf(strData, "Datum        : %s %02d.%02d", WeekDay[weekday(ForecastTime) - 1], day(ForecastTime), month(ForecastTime));
			Serial.println(strData);
			Serial.print("Stadt        : ");
			Serial.println(strCityNameForecast);
			Serial.print("Tages Temp   : ");
			Serial.print(tempDayForecast[i]);
			Serial.println("°C");
			Serial.print("Temp (Min)   : ");
			Serial.print(tempMinForecast[i]);
			Serial.println("°C");
			Serial.print("Temp (Max)   : ");
			Serial.print(tempMaxForecast[i]);
			Serial.println("°C");
			Serial.print("Feuchtigkeit : ");
			Serial.print(humidityForecast[i]);
			Serial.println("%");
			Serial.print("Wetter       : ");
			Serial.println(strWeatherForecast[i]);
			Serial.print("Icon Name    : ");
			Serial.println(strIconForecast[i]);
			Serial.println("----------------------------------------------");
		}
		
		// clear middle area
		tft.fillRect(5, yMiddle + 5, tftWidth - 10, hMiddle - 10, TFT_BLACK);
		tft.setFreeFont(DefaultFont);
		tft.setTextSize(1);	
		tft.setTextColor(TFT_YELLOW, TFT_BLACK);

		for (int i=0; i<u16Count; i++) {
			yield();
			tft.setFreeFont(DefaultFont);
			tft.drawCentreString(String(cDay[i]), 8 + ((i * 80) + 32), yMiddle + 8, 1);
			showWeather(strIconForecast[i], 8 + (i * 80), yMiddle + 30);
			tft.drawCentreString(String(tempMaxForecast[i], 1), 8 + ((i * 80) + 32), yMiddle + 8 + 64 + 24, 1);
		}
		
		Serial.print("memory used : ");
		Serial.println(jsonWeatherForecast.memoryUsage());
	}
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
// interrupt switch 1 and 2 - only for test
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