// ---------------------------------------------------------------------------------------------------
// Autor : Peter Schneider
// Datum : 23.01.2021 - change to VSCode and PlatformIO
//
// D1_mini with TFT 2.2", 320 x 240, SPI
// ---------------------------------------------------------------------------------------------------
// VCC        - 3V3
// GND        - GND
// CS         -  D1   - GPIO-05
// RESET      - RST
// D/C        -  D4   - GPIO-02
// SDI / MOSI -  D7   - GPIO-13
// SCK        -  D5   - GPIO-14
// LED        -  D3   - GPIO-00 TFT backligth with PWM
// SDO / MISO -  D6   - GPIO-12 there is a seccond use - switch 1
// ---------------------------------------------------------------------------------------------------
// LDR        -  A0   - ADC0    measure brigthness to change TFT backligth 
// Buzzer     -  D0	  - GPIO-16
// Switch 1   -  D6	  - GPIO-12
// Switch 2   -  D8   - GPIO-15
// LED        -  D2   - GPIO-04
// ---------------------------------------------------------------------------------------------------
// to convert fonts  to *.h : http://oleddisplay.squix.ch/#/home
// to convert images to *.h : http://rinkydinkelectronics.com/t_imageconverter565.php?msclkid=b9c2d4cdbd8811ec8ed10cdb25a780d0
// to convert and scale fonts to *.h use "fontconvert" a part of the Adafruit GFX graphics core library
// ---------------------------------------------------------------------------------------------------
#include "TFT22-Uhr.h"

// ---------------------------------------------------------------------------------------------------
// config.h include the definition of API_KEY and CITY_KEY
// #define API_KEY "1234567890abcdefghijklmnopqrst..."
// #define CITY_NAME_1 "Ort,DE"
// #define CITY_NAME_2 "Ort,DE"
// ---------------------------------------------------------------------------------------------------
#include "config.h"   
// ---------------------------------------------------------------------------------------------------

#define MY_TZ "CET-1CEST,M3.5.0,M10.5.0/3"			// timezone with buzzertime and wintertime 
#define MY_NTP_SERVER "europe.pool.ntp.org" 		// used ntp timeserver

time_t actualTime;
struct tm timeinfo;

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite actTimeToShow    = TFT_eSprite(&tft); 	// sprite to show acrual time
TFT_eSprite actDateToShow    = TFT_eSprite(&tft);	// sprite to show actual date
TFT_eSprite actTimeSecToShow = TFT_eSprite(&tft);	// sprite to show actual time and sec.

// use openweather setup
String ApiKey = API_KEY;
String CityName1 = CITY_NAME_1;
String CityName2 = CITY_NAME_2;

const char Server[] PROGMEM = "api.openweathermap.org";
String strIcon;

const char WeekDay[7][5] PROGMEM = {"So. ", "Mo. ", "Di. ", "Mi. ", "Do. ", "Fr. ", "Sa. "};

const GFXfont *DefaultFont = &Arimo10pt7b;
const GFXfont *TimeFont = &Arimo_Bold_Time54pt7b;

const uint16_t hSpace = 8;						// space

const uint16_t yTop = 0;					 	// start upper area
const uint16_t hTop = 35;						// higth of upper area

const uint16_t hBottom = 35;					// higth of lower area
const uint16_t yBottom = 240 - hBottom;			// start lower area

const uint16_t yMiddle = hTop + hSpace;			// start middle area
const uint16_t hMiddle = (240 - hTop - hBottom) - (2 * hSpace);		// higth of middle area

const uint16_t xPosWeatherNow = 2 * hSpace;
const uint16_t yPosWeatherNow = yBottom - (hSpace + 2) - 64;

uint16_t tftWidth;
uint16_t tftHeight;

WiFiManager wifiManager;
WiFiClient wifiClient;
ESP8266WebServer wifiServer(80);

clOut led;
clOut buzzer;

// definition of switch 1
stInput ParamSw01 = {SW_01, CHANGE, 40, 2000, irqSw01, POLARITY::POS, false};
clIn sw01; 			

// definition of switch 2
stInput ParamSw02 = {SW_02, CHANGE, 40, 2000, irqSw02, POLARITY::POS, false};
clIn sw02;

char cVersion[] PROGMEM = "02.00";
char cDatum[]   PROGMEM = __DATE__;

// definition of alarm times
stAlarmTime stWz[MAX_WECKER] = {
 	{WEEK_DAY::MO, 8, 00},
	{WEEK_DAY::DI, 8, 00},
	{WEEK_DAY::MI, 8, 00},
	{WEEK_DAY::DO, 8, 00},
	{WEEK_DAY::FR, 8, 00},
	{WEEK_DAY::SA, 8, 00},
	{WEEK_DAY::SO, 8, 00},
};

// definition of arlam clocks
std::array<clAlarm, MAX_WECKER> Wecker = {
    clAlarm(&timeinfo, &buzzer, &sw02, &stWz[0]),
    clAlarm(&timeinfo, &buzzer, &sw02, &stWz[1]),
    clAlarm(&timeinfo, &buzzer, &sw02, &stWz[2]),
    clAlarm(&timeinfo, &buzzer, &sw02, &stWz[3]),
    clAlarm(&timeinfo, &buzzer, &sw02, &stWz[4]),
    clAlarm(&timeinfo, &buzzer, &sw02, &stWz[5]),
    clAlarm(&timeinfo, &buzzer, &sw02, &stWz[6]),
};
typedef struct {
	char strStunden[3];
	char strMinuten[3];
	char strTage[2];  
	char strAktiv[2];
} alarm_data_t;

alarm_data_t WeckerDaten[MAX_WECKER] = {
	{"00", "00", "0", " "},
	{"00", "00", "1", " "},
	{"00", "00", "2", " "},
	{"00", "00", "3", " "},
	{"00", "00", "4", " "},
	{"00", "00", "5", " "},
	{"00", "00", "6", " "}
};

bool shouldSaveConfig = false;

menue_t hmMenue[8] = { 
//   function                   menue string             last item
	{runMainMenue,    	String(" "),                     false},		// 0
	{runAlarmTime_1, 	String("Weckzeit 0 einstellen"), false},		// 1	
	{runAlarmTime_2, 	String("Weckzeit 1 einstellen"), false},		// 2
	{runWeatherForcast, String("Wettervorschau 1"),      false},		// 3
	{runWeatherForcast, String("Wettervorschau 2"),      false},		// 4
	{runStartStopAlarm, String("Weckzeiten einsellen"),  false},		// 5
	{runState,        	String("Statusanzeige"),         false},		// 6
	{runDeleteFile,   	String("Delete Konfiguration"),   true}			// 7
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
	initOTA();
	
	tftBrigthnees();
	
	showFrame();
	
	configTime(MY_TZ, MY_NTP_SERVER);

	initFs();	// read config data of clock
	
	Serial.println();
	Serial.println(F("--------------------------------------"));
	Serial.println(F("- TFT2.2 clock spi                   -"));
	Serial.println(F("--------------------------------------"));
	Serial.println(String("version        - ") + String(cVersion));
	Serial.println(String("build date     - ") + String(cDatum));
	Serial.println(String("esp core       - ") + ESP.getCoreVersion());
	Serial.println(String("Free Heap Size - ") + ESP.getFreeHeap());
	Serial.println();

	initIrq();
	showWeatherIcon(bild_44, xPosWeatherNow, yPosWeatherNow);
}

// ---------------------------------------------------------------------------------------------------
// loop
// ---------------------------------------------------------------------------------------------------
void loop(void) {
	String strText = String(tempToday, 1) + String("'C - ") + String(humidityToday, 0) + String("% - ") + String(pressureToday, 0) + String("hPa");
	static String strTextOld = " ";

	ArduinoOTA.handle(); 					// OTA Upload via ArduinoIDE
	
	sw01.runState();
	sw02.runState();

	led.SwPort(sw01.Status());				// switch LED on with swith 1 - only for test
	tftBrigthnees();

	time(&actualTime);					 	// get actual time
	localtime_r(&actualTime, &timeinfo); 	// write actual time to timeinfo 

	clAlarm::Check();						// check alarm time

	showDateAndTime(timeinfo); 				
	showAlarmTime(false);
	
	HMenue.runMenue();					// run menue
	
	if (HMenue.getAktualMenue() != 0) {
		strTextOld = " ";
	}

	wifiServer.handleClient();

	// show clock and weather only at menue piont 0
	switch (HMenue.getAktualMenue()) {
		case 0:
			if (strTextOld != strText) {
				strTextOld = strText;
				showState(strText);
			}
		case 1:
		case 2:
		case 5:
		case 6:
		case 7:
			showTime(timeinfo, false);
			if (bGetWeather) {
				getActualWeather();	
				showWeather(strIconToday, xPosWeatherNow, yPosWeatherNow);
				bGetWeather = false;
			}	
			break;
		case 3:
		case 4:
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
	uint16_t u16Width = 64;		// icon size
	uint16_t u16Higth = 64;
	
	tft.fillRect(_xpos, _ypos, u16Width, u16Higth, TFT_BLACK);	
	tft.pushImage(_xpos, _ypos, 64, 64, _image);
}

void showState(String _strData) {
	static String strOld = " ";

	tft.setFreeFont(DefaultFont);
	tft.setTextSize(1);	

	// clear actual string
	tft.setTextColor(TFT_BLACK, TFT_BLACK);
	tft.drawCentreString(strOld, tftWidth / 2, yBottom + 8, 1);

	// draw new string
	tft.setTextColor(TFT_YELLOW, TFT_BLACK);
	tft.drawCentreString(_strData, tftWidth / 2, yBottom + 8, 1);
	tft.setFreeFont(NULL);

	strOld = _strData;
}

// ---------------------------------------------------------------------------------------------------
// menue functions
// ---------------------------------------------------------------------------------------------------
bool runMainMenue(void) {
	return clAlarm::enableAlarmTime(&sw02);
} 

bool runStartStopAlarm(void) {
	static uint16_t u16Status = 0;
	static uint16_t u16StatusOld = 1;
	static uint16_t u16AlarmNumber = 0;

	static uint32_t u32Timeout = 0;
	
	bool bResult = false;

	if (u16Status != u16StatusOld) {
		Serial.println(TraceTime() + String("runStartStopAlarm - ") + String(u16Status));
		u16StatusOld = u16Status;
	}

	switch (u16Status) {
		case 0:
			if (!sw02.Status() && !sw01.Status()) {	
				u16AlarmNumber = 0;
				u16Status = 5;
			}
			break;
		case 5:
			bResult = true;
			if (sw02.Status()) {
				u16Status = 10;
			} 
			break;
		case 10:
			if (!sw02.Status()) {
				showState(String("Weckzeit ") + String(u16AlarmNumber) + String(" ein / aus"));
				u32Timeout = millis();
				u16Status = 20;
			}
			break;
		case 20:
			if (Wecker[u16AlarmNumber].setStartStopAlarm()) {
				bResult = true;
				saveWeckerConfig();
				u16Status = 0;
			} else if (millis() > (u32Timeout + 3000)) {
				bResult = true;
				u16Status = 0;
			} 
			
			if (sw01.Status()) {
				if (++u16AlarmNumber >= MAX_WECKER) {
					u16AlarmNumber = 0;
				}
				u16Status = 30;
			} 
			break;
		case 30:
			if (!sw01.Status()) {
				u16Status = 10;
			}
			break;
		default:
			break;
	}
	
	return bResult;
}

bool changeAlarmTime(uint16_t _u16Nr) {
	static uint16_t u16Status = 0;
	static uint16_t u16StatusOld = 1;
		
	bool bResult = false;

	if (u16Status != u16StatusOld) {
		Serial.println(TraceTime() + String("changeAlarmTime - ") + String(u16Status));
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
				if (Wecker[_u16Nr].setNewAlarmTime()) {
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

bool runAlarmTime_1(void) {
	return changeAlarmTime(0);
}
bool runAlarmTime_2(void) {
	return changeAlarmTime(1);
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
				Serial.printf("delete file: %s\r\n", cFile);
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
	String strData;
	
	switch (u16Status) {
		case 0:
			bResult = true;
			if (sw02.Status()) {
				Serial.println("get weather forcast");
				showState("lade Wetterdaten");
				u16Status = 10;
			}
			break;
		case 10:
			getWeatherForcast();
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
				tft.fillRect(5, yMiddle + 5, tftWidth - 10, hMiddle - 10, TFT_BLACK);	// clear screen 
				showAlarmTime(true);
				showTime(timeinfo, true);
				bGetWeather = true;														// load weather icon
				showWeather(strIconToday, xPosWeatherNow, yPosWeatherNow);
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
	uint16_t spriteHigth = 18;
	
	// date
	uint16_t spriteWidth1 = (tftWidth / 2) - 20;
	uint16_t spriteXPos1  = hSpace;	
	uint16_t spriteYPos1  = 9;

	// time
	uint16_t spriteWidth2 = (tftWidth / 2) - 75;
	uint16_t spriteXPos2  = (tftWidth / 2) + 75 - hSpace;	
	uint16_t spriteYPos2  = 9;

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

void showAlarmTime(bool _bForce) {
	static String oldString[MAX_WECKER];
	String strAlarmTime[MAX_WECKER];
	String strName;
	String strHour;
	String strMinute;
	String strDay;

	uint16_t xOffset = 64 + (2 * hSpace);
	uint16_t yFontHeight = 21;
	uint16_t xFontWidth = 10;

	tft.setFreeFont(DefaultFont);
	tft.setTextSize(1);	
	tft.setTextColor(TFT_YELLOW, TFT_BLACK);

//	for (int i = 0; i < MAX_WECKER; i++) {
	for (int i = 0; i < 2; i++) {
		uint16_t y = 141 + (i * yFontHeight + 2); 

		//   0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18
		//   W 0 :   *   0 5 : 4 5     :  M  o
		strAlarmTime[i] = Wecker[i].getTimeString(); 		
		strName   = strAlarmTime[i].substring(0,5);	 			// "W0: * "		
		strHour   = strAlarmTime[i].substring(6,8);				// "05"
		strMinute = strAlarmTime[i].substring(9,11);			// "45"
		strDay    = strAlarmTime[i].substring(14,strAlarmTime[i].length());

		if ((oldString[i] != strAlarmTime[i]) || _bForce) {
			Serial.println(TraceTime() + strAlarmTime[i]);

			tft.drawString(strName, xOffset + 10, y);						// name
			if (strName.substring(4,5) == " ") {
				tft.fillRect(xOffset + (xFontWidth * 5), y, xFontWidth, yFontHeight, TFT_BLACK);
			}
			
			if (strHour.substring(0,1) != " ") {
				tft.drawString(strHour, xOffset + (xFontWidth * 7), y);		// hour
			} else {
				tft.fillRect(xOffset + (xFontWidth * 7), y, (xFontWidth * 3), yFontHeight, TFT_BLACK);
			}
			tft.drawString(":", xOffset + 98, y);				// :
			if (strMinute.substring(0,1) != " ") {			
				tft.drawString(strMinute, xOffset + (xFontWidth * 11), y);	// minute
			} else {
				tft.fillRect(xOffset + (xFontWidth * 11), y, (xFontWidth * 3), yFontHeight, TFT_BLACK);
			}
			tft.drawString(" - ", xOffset + 138, y);			// -
			if (strDay.substring(0,1) != " ") {
				tft.drawString(strDay, xOffset + (xFontWidth * 16), y);		// day
			} else {
				tft.fillRect(xOffset + (xFontWidth * 16), y, (xFontWidth * 7), yFontHeight, TFT_BLACK);
			}
			
			oldString[i] = strAlarmTime[i];
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
	static tm tmTimeOld;
 
	uint16_t spriteWidth = tftWidth - (4 * hSpace);
	uint16_t spriteHigth = 81;
	uint16_t spriteYPos  = yMiddle + (hSpace / 2);
	uint16_t spriteXPos  = tftWidth - spriteWidth - (2 * hSpace);	

	char str[8];
	String strZeit;
	String strInfo = "wait for NTP and weather";

	switch (u16State)  	{
		case 0: 	// init time if ntp call is ready for the first time
			actTimeToShow.setColorDepth(8);
			actTimeToShow.createSprite(spriteWidth, spriteHigth);
			actTimeToShow.setFreeFont(DefaultFont);
			actTimeToShow.setTextSize(1);		
			actTimeToShow.fillSprite(TFT_BLACK);

			actTimeToShow.setTextColor(TFT_YELLOW);
			actTimeToShow.drawCentreString(strInfo, spriteWidth / 2, (spriteHigth / 2), 1);
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
			if ((_actTimeinfo.tm_sec == 0) || _bForce || (tmTimeOld.tm_min != _actTimeinfo.tm_min)) {
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
			actTimeToShow.fillSprite(TFT_BLACK);
			
			// write new time
			actTimeToShow.setTextColor(TFT_YELLOW);
			actTimeToShow.drawCentreString(strZeit, spriteWidth / 2, 0, 1);
			
			// show sprite at screen
			actTimeToShow.pushSprite(spriteXPos, spriteYPos);

			tmTimeOld = _actTimeinfo;
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

	// buzzer
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
		wifiServer.begin();
		delay(4000);
	} else {
		tft.drawString(".. WLan error !!", 10, 40);
		tft.drawString(".. ESP Reset !!", 10, 70);
		delay(2000);
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
						stWz[i].Wochentag = (WEEK_DAY)Data.toInt();

						Wecker[i].setTime(&stWz[i]);
						if (WeckerDaten[i].strAktiv[0] == '*') {
							Serial.println((String)".. Wecker " + String(i) + (String)" ist aktiv");
							Wecker[i].Start();
						}	
					}					
					Serial.print(F("memory used : "));
					Serial.println(json.memoryUsage());
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
	String strUrl = "GET /data/2.5/weather?q=" + CityName1 + "&appid=" + ApiKey + "&lang=de&mode=json&units=metric";
	
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

		Serial.println(F("----------------------------------------------"));
		Serial.print(F("Stadt        : "));
		Serial.println(strCityNameToday);
		Serial.print(F("Aktuelle Temp: "));
		Serial.print(tempToday);
		Serial.println(F("°C"));
		Serial.print(F("Temp (Min)   : "));
		Serial.print(tempMinToday);
		Serial.println(F("°C"));
		Serial.print(F("Temp (Max)   : "));
		Serial.print(tempMaxToday);
		Serial.println(F("°C"));
		Serial.print(F("Feuchtigkeit : "));
		Serial.print(humidityToday);
		Serial.println(F("%"));
		Serial.print(F("Wetter       : "));
		Serial.println(strWeatherToday);
		Serial.print(F("Icon Name    : "));
		Serial.println(strIconToday);
		Serial.println(F("----------------------------------------------"));

		Serial.print(F("memory used : "));
		Serial.println(jsonWeatherToday.memoryUsage());
	}
}

void getWeatherForcast(void) {
	String strUrl;
	
	if (HMenue.getAktualMenue() == 3) {
		strUrl = "GET /data/2.5/forecast/daily?q=" + CityName1 + "&appid=" + ApiKey + "&cnt=4&lang=de&mode=json&units=metric";
	} else {
		strUrl = "GET /data/2.5/forecast/daily?q=" + CityName2 + "&appid=" + ApiKey + "&cnt=4&lang=de&mode=json&units=metric";
	}
	
	Serial.println(TraceTime() + "getWeatherForcast");
	Serial.println(TraceTime() + strUrl);

	decodeWeatherForcast(getJsonDataFromWeb(Server, strUrl));
}

void decodeWeatherForcast(String _WetterDaten) {
	Serial.println(String("Free Heap Size - ") + ESP.getFreeHeap());
	DynamicJsonDocument jsonWeatherForecast(2500);
	Serial.println(String("Free Heap Size - ") + ESP.getFreeHeap());
	
	uint16_t u16Count = 0;
	time_t ForecastTime;
	char strData[30];
	char cDay[FORECAST][5];
	
	Serial.println(TraceTime() + "decodeWeatherForcast");
	
	DeserializationError error = deserializeJson(jsonWeatherForecast, _WetterDaten);

	if (error) {
		Serial.print(TraceTime() + "deserializeJson failed - ");
		Serial.println(error.c_str());
		showState(error.c_str());
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

			Serial.println(F("----------------------------------------------"));
			sprintf(strData, "Datum        : %s %02d.%02d", WeekDay[weekday(ForecastTime) - 1], day(ForecastTime), month(ForecastTime));
			Serial.println(strData);
			Serial.print(F("Stadt        : "));
			Serial.println(strCityNameForecast);
			Serial.print(F("Tages Temp   : "));
			Serial.print(tempDayForecast[i]);
			Serial.println(F("°C"));
			Serial.print(F("Temp (Min)   : "));
			Serial.print(tempMinForecast[i]);
			Serial.println(F("°C"));
			Serial.print(F("Temp (Max)   : "));
			Serial.print(tempMaxForecast[i]);
			Serial.println(F("°C"));
			Serial.print(F("Feuchtigkeit : "));
			Serial.print(humidityForecast[i]);
			Serial.println(F("%"));
			Serial.print(F("Wetter       : "));
			Serial.println(strWeatherForecast[i]);
			Serial.print(F("Luftdruck    : "));
			Serial.print(pressureForecast[i]);
			Serial.println(F("hPa"));
			Serial.print(F("Icon Name    : "));
			Serial.println(strIconForecast[i]);
			Serial.println(F("----------------------------------------------"));
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
			tft.drawCentreString(String(tempMaxForecast[i], 0) + String("'C"), 8 + ((i * 80) + 32), yMiddle + 8 + 64 + 24, 1);
			tft.drawCentreString(String(humidityForecast[i], 0) + String("%"), 8 + ((i * 80) + 32), yMiddle + 8 + 64 + 48, 1);
		}
		
		Serial.print(F("memory used : "));
		Serial.println(jsonWeatherForecast.memoryUsage());

		showState(convertStringToGerman(String("Wetter in ") + strCityNameForecast));
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

String convertStringToGerman(String strData) {
	strData.replace(String("ß"), String("&"));
	strData.replace(String("ü"), String("("));
	strData.replace(String("°"), String("'"));
	return strData;
}

void initOTA(void) {
	ArduinoOTA.onStart([]() {  
	 	tft.fillScreen(TFT_BLACK);
		tft.setFreeFont(DefaultFont);
		tft.setTextColor(TFT_WHITE, TFT_BLACK);
		tft.setCursor(0,30);
		tft.println(F(".. Start Update"));
	});
 	ArduinoOTA.onEnd([]() {  
		tft.println();
		tft.println(F(".. Restart System"));
		delay(2000);
  	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		static uint16_t u16FirstCall = true;
		static uint16_t u16Count = 0;
		if (u16FirstCall) {
			tft.print(F(".. Progress: "));
			u16FirstCall = false;
		} else {
			if (u16Count++ == 25) {
				tft.print(".");
				u16Count = 0;
			}
			if (total == progress) {
				tft.println();
				tft.print(F(".. Update ready"));
			}
		}
	});
	ArduinoOTA.begin(); 	
}
