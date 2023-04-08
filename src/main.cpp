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

TFT_eSPI tft = TFT_eSPI();

const GFXfont *DefaultFont = &Arimo10pt7b;
const GFXfont *TimeFont = &Arimo_Bold_Time54pt7b;

TFT_eSprite actTimeToShow    = TFT_eSprite(&tft); 	// sprite to show acrual time
TFT_eSprite actDateToShow    = TFT_eSprite(&tft);	// sprite to show actual date
TFT_eSprite actTimeSecToShow = TFT_eSprite(&tft);	// sprite to show actual time and sec.

#define MY_TZ "CET-1CEST,M3.5.0,M10.5.0/3"			// timezone with buzzertime and wintertime 
#define MY_NTP_SERVER "europe.pool.ntp.org" 		// used ntp timeserver

time_t actualTime;
struct tm timeinfo;

// use openweather setup
char ApiKey[34];
char CityName1[20];
char CityName2[20];

const char Server[] PROGMEM = "api.openweathermap.org";
char strIcon[6];

const char WeekDay[7][5] PROGMEM = {"So. ", "Mo. ", "Di. ", "Mi. ", "Do. ", "Fr. ", "Sa. "};

WiFiManager wifiManager;
WiFiClient wifiClient;
ESP8266WebServer wifiServer(80);

// Set the username and password for the webserver
const char* http_username = "admin";
const char* http_password = "13579";
const char* cDnsName = "ESPWecker01";

clOut led;
clOut buzzer;

// definition of switch 1
stInput ParamSw01 = {SW_01, CHANGE, 40, 2000, irqSw01, POLARITY::POS, false};
clIn sw01; 			

// definition of switch 2
stInput ParamSw02 = {SW_02, CHANGE, 40, 2000, irqSw02, POLARITY::POS, false};
clIn sw02;

char cVersion[] PROGMEM = "03.01";
char cDatum[]   PROGMEM = __DATE__;

// definition of alarm times
stAlarmTime stWz[MAX_WECKER] = {
 	{WEEK_DAY::MO, 8, 00, false},
	{WEEK_DAY::DI, 8, 00, false},
	{WEEK_DAY::MI, 8, 00, false},
	{WEEK_DAY::DO, 8, 00, false},
	{WEEK_DAY::FR, 8, 00, false},
	{WEEK_DAY::SA, 8, 00, false},
	{WEEK_DAY::SO, 8, 00, false}
};

// definition of arlam clocks
std::array<clAlarm, MAX_WECKER> Wecker = {
    clAlarm(&timeinfo, &buzzer, &sw02, &stWz[0]),
    clAlarm(&timeinfo, &buzzer, &sw02, &stWz[1]),
    clAlarm(&timeinfo, &buzzer, &sw02, &stWz[2]),
    clAlarm(&timeinfo, &buzzer, &sw02, &stWz[3]),
    clAlarm(&timeinfo, &buzzer, &sw02, &stWz[4]),
    clAlarm(&timeinfo, &buzzer, &sw02, &stWz[5]),
    clAlarm(&timeinfo, &buzzer, &sw02, &stWz[6])
};

bool shouldSaveConfig = false;

menue_t hmMenue[8] = { 
//   function           menue string             last item
	{runMainMenue,    	" ",                     false},		
	{changeAlarmTime, 	"Weckzeit 0 einstellen", false},			
	{changeAlarmTime, 	"Weckzeit 1 einstellen", false},		
	{runWeatherForcast, "Wettervorschau 1",      false},		
	{runWeatherForcast, "Wettervorschau 2",      false},		
	{runState,        	"Statusanzeige",         false},		
	{runDeleteFile,   	"Delete Konfiguration",   true}			
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

char strDummy[30];

// ---------------------------------------------------------------------------------------------------
// setup
// ---------------------------------------------------------------------------------------------------
void setup() {
	snprintf_P(ApiKey, sizeof(ApiKey), PSTR(API_KEY));
	snprintf_P(CityName1, sizeof(CityName1), PSTR(CITY_NAME_1));
	snprintf_P(CityName2, sizeof(CityName2), PSTR(CITY_NAME_2));

	Serial.begin(115200);
    Serial.println();

	initDisplay();
	initGpio();
	initNetwork();
	initOTA();

	Serial.println();
	Serial.println(F("--------------------------------------"));
	Serial.println(F("- TFT2.2 clock spi                   -"));
	Serial.println(F("--------------------------------------"));

	snprintf_P(strDummy, sizeof(strDummy), PSTR("version        - %s"), cVersion);
	Serial.println(String(strDummy));
	snprintf_P(strDummy, sizeof(strDummy), PSTR("build date     - %s"), cDatum);
	Serial.println(String(strDummy));
	Serial.print(String(F("esp core       - ")));
	Serial.println(ESP.getCoreVersion());
	Serial.print(String(F("free heap size - ")));
	Serial.println(ESP.getFreeHeap());

	tftBrigthnees();
	showFrame();
	configTime(MY_TZ, MY_NTP_SERVER);

	initIrq();
	initFs();	// read config data of clock

	showWeatherIcon(bild_44, X_POS_WEATHER_NOW, Y_POS_WEATHER_NOW);
}

// ---------------------------------------------------------------------------------------------------
// loop
// ---------------------------------------------------------------------------------------------------
void loop(void) {
	char strText[40];
	static char strTextOld[40] = " ";

	snprintf_P(strText, sizeof(strText), PSTR("%2.1f'C - %i%s - %ihPa"), tempToday, (int)humidityToday, "%", (int)pressureToday);

	ArduinoOTA.handle(); 					// OTA Upload via ArduinoIDE
	wifiServer.handleClient();
	
	sw01.runState();
	sw02.runState();

	led.SwPort(sw01.Status());				// switch LED on with swith 1 - only for test

	tftBrigthnees();

	time(&actualTime);					 	// get actual time
	localtime_r(&actualTime, &timeinfo); 	// write actual time to timeinfo 
	
	clAlarm::Check();						// check alarm time
	clAlarm::NextAlarm();					// check for next alarm

	showDateAndTime(timeinfo); 				// line one to show date and time 		
	showAlarmTime(false);					
	HMenue.handle();						// run menue
	
	if (HMenue.getAktualMenue() != 0) {
		memset(strTextOld, 0, sizeof(strTextOld));
	}

	// show clock and weather only at menue number 0
	switch (HMenue.getAktualMenue()) {
		case 0:
			if (strcmp(strTextOld, strText) != 0) {
				strcpy(strTextOld, strText);
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
				showWeather(strIconToday.c_str(), X_POS_WEATHER_NOW, Y_POS_WEATHER_NOW);
				bGetWeather = false;
			}	
			break;
		case 3:
		case 4:
			break;
	}		   	
}

// ---------------------------------------------------------------------------------------------------
// menue functions
// ---------------------------------------------------------------------------------------------------
bool runMainMenue(void) {
	return clAlarm::enableAlarmTime(&sw02);
} 

bool changeAlarmTime(void) {
	static uint16_t u16Status = 0;
	static uint16_t u16StatusOld = 1;
		
	bool bResult = false;

	uint16_t _u16Nr;
	
	switch (HMenue.getAktualMenue()) {
		case 1: _u16Nr = 0; break;
		case 2: _u16Nr = 1; break;
		default: _u16Nr = 0; break;
	}

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
				showState("config. deleted");
			} else {
				showState("delete error");
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
				tft.fillRect(5, Y_MIDDLE + 5, DISP_WIDTH - 10, H_MIDDLE - 10, TFT_BLACK);	// clear screen 
				showAlarmTime(true);
				showTime(timeinfo, true);
				bGetWeather = true;														// load weather icon
				showWeather(strIconToday.c_str(), X_POS_WEATHER_NOW, Y_POS_WEATHER_NOW);
				u16Status = 0;
			}
			break;
		default:
			u16Status = 0;
			break;
	}

	return bResult;
}

void showDateAndTime(struct tm _actTimeinfo) {
	uint16_t spriteHigth = 18;
	
	// date
	uint16_t spriteWidth1 = (DISP_WIDTH / 2) - 20;
	uint16_t spriteXPos1  = H_SPACE;	
	uint16_t spriteYPos1  = 9;

	// time
	uint16_t spriteWidth2 = (DISP_WIDTH / 2) - 75;
	uint16_t spriteXPos2  = (DISP_WIDTH / 2) + 75 - H_SPACE;	
	uint16_t spriteYPos2  = 9;

	char str[20];

	static String strTimeOld = " ";
	String strTime;
	
	static String strDateOld = " ";
	String strDate;
		
	snprintf_P(str, sizeof(str), PSTR("%02u:%02u:%02u"), _actTimeinfo.tm_hour, _actTimeinfo.tm_min, _actTimeinfo.tm_sec);
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

	snprintf_P(str, sizeof(str), PSTR("%s"), WeekDay[_actTimeinfo.tm_wday]);
	
	strDate = String(str);
	snprintf_P(str, sizeof(str), PSTR("%02u.%02u.%4u"), _actTimeinfo.tm_mday, _actTimeinfo.tm_mon + 1, 1900 + _actTimeinfo.tm_year);
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

	uint16_t xOffset = 64 + (2 * H_SPACE);
	uint16_t yFontHeight = 21;
	uint16_t xFontWidth = 10;

	tft.setFreeFont(DefaultFont);
	tft.setTextSize(1);	
	tft.setTextColor(TFT_YELLOW, TFT_BLACK);

	for (int i = 0; i < 2; i++) {
		uint16_t y = 141 + ((i * yFontHeight) + 2); 
	
		//   0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
		//   W 0 :   *   0 5 : 4 5   :        M  o     -  F  r
		strAlarmTime[i] = Wecker[i].getTimeString(); 		
		strName   = strAlarmTime[i].substring(0,6);	 			// "W0: * "		
		strHour   = strAlarmTime[i].substring(6,8);				// "05"
		strMinute = strAlarmTime[i].substring(9,11);			// "45"
		strDay    = strAlarmTime[i].substring(14);				// "Mo"
		
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
 
	uint16_t spriteWidth = DISP_WIDTH - (4 * H_SPACE);
	uint16_t spriteHigth = 81;
	uint16_t spriteYPos  = Y_MIDDLE + (H_SPACE / 2);
	uint16_t spriteXPos  = DISP_WIDTH - spriteWidth - (2 * H_SPACE);	

	char str[8];
	String strZeit;
	String strInfo = "wait for weather data";

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
			snprintf_P(str, sizeof(str), PSTR("%u:%02u"), _actTimeinfo.tm_hour, _actTimeinfo.tm_min);
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
	static uint32_t authenticationTimer = 0;

	if (++weatherTimer >= 60 * 15) {
		weatherTimer = 0;
		bGetWeather = true;
	}

	// reset authentication after 3 min.
	if (getAuthentication()) {
		if (++authenticationTimer >= 60 * 3) {
			clearAuthentication();
		}
	} else {
		authenticationTimer = 0;
	}

	timer0_write(ESP.getCycleCount() + 80000000L); // for 80MHz this means 1 interrupt per seccond
}

// -----------------------------------------------------------------------------------
// init GPIO
// -----------------------------------------------------------------------------------
void initGpio(void) {
	Serial.println("-- init gpio");
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
	Serial.println("-- init dsplay");	
	tft.init();
	tft.setSwapBytes(true);				// used by push image function
	tft.setRotation(ROTATION_90);
	tft.fillScreen(TFT_BLACK);
	tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

// -----------------------------------------------------------------------------------
// init network
// -----------------------------------------------------------------------------------
void initNetwork(void) {
	wifiManager.setAPCallback(wifiCallback);
	wifiManager.setSaveConfigCallback(saveConfigCallback);

	tft.fillScreen(TFT_BLACK);
	tft.setCursor(0, 30);
	tft.setFreeFont(DefaultFont);
	tft.setTextSize(1);	
	tft.drawString(".. start WLan", 10, 10);

	if (wifiManager.autoConnect("ESP_TFT_UHR")) {
		tft.drawString(".. WLan connected", 10, 40);
		String strText = String(".. ") + WiFi.SSID() + String(" - ") + WiFi.localIP().toString();
		tft.drawString(strText, 10, 70);

		MDNS.begin(cDnsName);

		wifiServer.on("/", handleIndex);
		wifiServer.on("/values", HTTP_GET, handleValues);
	  	wifiServer.on("/config", handleConfig);
		wifiServer.on("/delete", handleDelete);
		wifiServer.on("/weather", handleWeather);
		wifiServer.on("/Authentication", handleAuthentication);
		wifiServer.on("/logout", handleLogout);
		wifiServer.begin();
		Serial.println(".. server online");
		delay(4000);
	} else {
		tft.drawString(".. WLan error !!", 10, 40);
		tft.drawString(".. ESP Reset !!", 10, 70);
		delay(2000);
		while (true) {;}
	}
}

void wifiCallback(WiFiManager *_myWiFiManager) {
	tft.fillScreen(TFT_BLACK);
	tft.println(" ");
	tft.println(".. Konfig. Mode aktiv");
	tft.print(".. ");
	tft.println(WiFi.softAPIP());
	tft.println(String(".. ") + _myWiFiManager->getConfigPortalSSID());
}

// -----------------------------------------------------------------------------------
// init file system and read configuration
// -----------------------------------------------------------------------------------
void initFs(void) {
	Serial.println(".. mounting FS");

  	if (LittleFS.begin()) {

		//return;  

   		Serial.println(".. FS mounted");
    	if (LittleFS.exists("/config.json")) {
      		// file exists, reading and loading
      		Serial.println(".. open config file");
      		File configFile = LittleFS.open("/config.json", "r");
      		if (configFile) {
        		Serial.println(".. config file readed");
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
					char strMaxWecker[5];

					snprintf_P(strData, sizeof(strData), PSTR("MaxWecker"));
					strcpy(strMaxWecker, json[strData]);
					String Data = (String)strMaxWecker;
					int iMaxWecker = Data.toInt();
					
					for (int i = 0; i < iMaxWecker; i++) {
						snprintf_P(strData, sizeof(strData), PSTR("WeckerStunden_%i") , i);
						Wecker[i].setNewAlarmHour(json[strData]);
												
						snprintf_P(strData, sizeof(strData), PSTR("WeckerMinuten_%i") , i);
						Wecker[i].setNewAlarmMinute(json[strData]);
						
						snprintf_P(strData, sizeof(strData), PSTR("WeckerTage_%i") , i);
						Wecker[i].setNewWeekDay(json[strData]);
						
						snprintf_P(strData, sizeof(strData), PSTR("WeckerAktiv_%i") , i);
						if (json[strData] == String('*')) {
							Serial.println((String)".. Wecker " + String(i) + (String)" ist aktiv");
							Wecker[i].Start();
						} else {
							Wecker[i].Stop();
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
  shouldSaveConfig = true;
}

void saveWeckerConfig(void) {
	Serial.println("Konfiguration speichern");
	DynamicJsonDocument json(1024);
	char strData[20]; 
	
	snprintf_P(strData, sizeof(strData), PSTR("MaxWecker"));
	json[strData] = (String)MAX_WECKER;

	for(int i = 0; i < MAX_WECKER; i++) {
		snprintf_P(strData, sizeof(strData), PSTR("WeckerStunden_%i") , i);
		json[strData] = Wecker[i].getWeckStunde(); 
		
		snprintf_P(strData, sizeof(strData), PSTR("WeckerMinuten_%i") , i);
		json[strData] = Wecker[i].getWeckMinute();
		
		snprintf_P(strData, sizeof(strData), PSTR("WeckerTage_%i") , i);
		json[strData] = Wecker[i].getWeckTage(); 
		
		snprintf_P(strData, sizeof(strData), PSTR("WeckerAktiv_%i") , i);
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

	while (!wifiClient.available()) {
      delay(100);
    }

	return wifiClient.readString();
}

// -----------------------------------------------------------------------------------
// get weather data from internet - https:\\openweathermap.org
// -----------------------------------------------------------------------------------
void getActualWeather(void) {
	char strUrl[150];
	snprintf_P(strUrl, sizeof(strUrl), PSTR("GET /data/2.5/weather?q=%s&appid=%s&lang=de&mode=json&units=metric"), CityName1, ApiKey);
	
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
	char strUrl[150];
	
	if (HMenue.getAktualMenue() == 3) {
		snprintf_P(strUrl,sizeof(strUrl), PSTR("GET /data/2.5/forecast/daily?q=%s&appid=%s&cnt=4&lang=de&mode=json&units=metric"), CityName1, ApiKey);
	} else {
		snprintf_P(strUrl,sizeof(strUrl), PSTR("GET /data/2.5/forecast/daily?q=%s&appid=%s&cnt=4&lang=de&mode=json&units=metric"), CityName2, ApiKey);
	}
	
	Serial.println(TraceTime() + String("getWeatherForcast"));
	Serial.println(TraceTime() + String(strUrl));

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
			snprintf_P(strData, sizeof(strData), PSTR("Datum        : %s %02d.%02d"), WeekDay[weekday(ForecastTime) - 1], day(ForecastTime), month(ForecastTime));
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
		tft.fillRect(5, Y_MIDDLE + 5, DISP_WIDTH - 10, H_MIDDLE - 10, TFT_BLACK);
		tft.setFreeFont(DefaultFont);
		tft.setTextSize(1);	
		tft.setTextColor(TFT_YELLOW, TFT_BLACK);

		for (int i=0; i<u16Count; i++) {
			yield();
			tft.setFreeFont(DefaultFont);
			tft.drawCentreString(String(cDay[i]), 8 + ((i * 80) + 32), Y_MIDDLE + 8, 1);
			showWeather(strIconForecast[i].c_str(), 8 + (i * 80), Y_MIDDLE + 30);
			tft.drawCentreString(String(tempMaxForecast[i], 0) + String("'C"), 8 + ((i * 80) + 32), Y_MIDDLE + 8 + 64 + 24, 1);
			tft.drawCentreString(String(humidityForecast[i], 0) + String("%"), 8 + ((i * 80) + 32), Y_MIDDLE + 8 + 64 + 48, 1);
		}
		
		Serial.print(F("memory used : "));
		Serial.println(jsonWeatherForecast.memoryUsage());

		showState(convertStringToGerman(String("Wetter in ") + strCityNameForecast).c_str());
	}
}

// -----------------------------------------------------------------------------------
// init timer irq
// -----------------------------------------------------------------------------------
void initIrq(void) {
	Serial.println(".. init irq");
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
	snprintf_P(cTimeStampStr, sizeof(cTimeStampStr), PSTR("> %02u:%02u:%02u.%03u - "), u16h, u16m, u16s, u16ms);
	return String(cTimeStampStr);
}

String convertStringToGerman(String strData) {
	strData.replace(String("ß"), String("&"));
	strData.replace(String("ü"), String("("));
	strData.replace(String("°"), String("'"));
	return strData;
}

