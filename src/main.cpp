// ---------------------------------------------------------------------------------------------------
// Autor : Peter Schneider
// Datum : 23.01.2021 - change to VSCode and PlatformIO
//
// D1_mini / ESP8266-12 with TFT 2.2", 320 x 240, SPI
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
clDisplay* pTft = nullptr;  

const char* ntpServer = "pool.ntp.org";   // used ntp server 
const char* timezone  = "CET-1CEST,M3.5.0,M10.5.0/3";  // string for Europe / Berlin

tm actualTime;

// use openweather setup
char ApiKey[34];
char CityName1[20];
char CityName2[20];

const char Server[] PROGMEM = "api.openweathermap.org";
char strIcon[6];

const char WeekDay[7][5] PROGMEM = {"So. ", "Mo. ", "Di. ", "Mi. ", "Do. ", "Fr. ", "Sa. "};

WiFiManager*     	pWifiManager = nullptr;
WiFiClient*      	pWifiClient = nullptr;
ESP8266WebServer*	pWifiServer = nullptr;

// Set the username and password for the webserver
const char* http_username = "admin";
const char* http_password = "13579";
const char* cDnsName = "ESP_Wecker";

// definition of GPIO outputs
clOut* pLed = nullptr;
clOut* pBuzzer = nullptr;

// definition of switch 1
stInput ParamSw01 = {SW_01, CHANGE, 40, 2000, irqSw01, POLARITY::POS, false};
clIn* pSw01 = nullptr; 			

// definition of switch 2
stInput ParamSw02 = {SW_02, CHANGE, 40, 2000, irqSw02, POLARITY::POS, false};
clIn* pSw02 = nullptr;

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
std::array<clAlarm*, MAX_WECKER> Wecker = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

menue_t hmMenue[7] = { 
// function           menue string             last item
	{runMainMenue,    	" ",                     false},	// 0		
	{changeAlarmTime, 	"Weckzeit 0 einstellen", false},	// 1			
	{changeAlarmTime, 	"Weckzeit 1 einstellen", false},	// 2	
	{runWeatherForcast, "Wettervorschau 1",      false},	// 3	
	{runWeatherForcast, "Wettervorschau 2",      false},	// 4	
	{runState,        	"Statusanzeige",         false},	// 5	
	{runDeleteFile,   	"Delete Konfiguration",   true}		// 6	
};

// menue control
clMenue* pHMenue = nullptr;

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

uint32_t u32GetWeatherTimeout = 0;

char strDummy[30];

// ---------------------------------------------------------------------------------------------------
// setup
// ---------------------------------------------------------------------------------------------------
void setup() {
	Serial.begin(115200);
  Serial.println();

	// create classes
	pWifiManager = new WiFiManager;
	pWifiClient  = new WiFiClient;
	pWifiServer  = new ESP8266WebServer(80);

	pLed    = new clOut;
	pBuzzer = new clOut;
	pTft    = new clDisplay;
	pHMenue = new clMenue;
	pSw01   = new clIn;
	pSw02   = new clIn;

	initGpio();
	initDisplay();
	initNetwork();

	for (int i = 0; i < MAX_WECKER; i++) {
		Wecker[i] = new clAlarm;
		Wecker[i]->init(&actualTime, pBuzzer, pSw02, &stWz[i]);
 	}

	pHMenue->init(pSw01, hmMenue, pTft);
	
	snprintf_P(ApiKey, sizeof(ApiKey), PSTR(API_KEY));
	snprintf_P(CityName1, sizeof(CityName1), PSTR(CITY_NAME_1));
	snprintf_P(CityName2, sizeof(CityName2), PSTR(CITY_NAME_2));

	Serial.println();
	Serial.println(F("--------------------------------------"));
	Serial.println(F("- ESP8266 clock                      -"));
	Serial.println(F("--------------------------------------"));

	snprintf_P(strDummy, sizeof(strDummy), PSTR("version            - %s"), cVersion);
	Serial.println(String(strDummy));
	snprintf_P(strDummy, sizeof(strDummy), PSTR("build date         - %s"), cDatum);
	Serial.println(String(strDummy));
	Serial.print(String(F("esp core           - ")));
	Serial.println(ESP.getCoreVersion());
	Serial.print(String(F("esp core id        - ")));
	Serial.println(ESP.getChipId());
	Serial.print(String(F("free heap size     - ")));
	Serial.print(ESP.getFreeHeap());
	Serial.println(String(F(" byte")));
	Serial.print(String(F("free sktech memory - ")));
	Serial.print(ESP.getFreeSketchSpace());
	Serial.println(String(F(" byte")));

	if (SPIFFS.begin()) {		// mount 

		FSInfo fs_info;
		SPIFFS.info(fs_info);
		Serial.println(".. FS mounted");

		Serial.println("   blockSize:     " + (String)fs_info.blockSize);
		Serial.println("   maxOpenFiles:  " + (String)fs_info.maxOpenFiles);
		Serial.println("   maxPathLength: " + (String)fs_info.maxPathLength);
		Serial.println("   pageSize:      " + (String)fs_info.pageSize);
		Serial.println("   totalBytes:    " + (String)fs_info.totalBytes);
		Serial.println("   usedBytes:     " + (String)fs_info.usedBytes);
	} else {
		Serial.println(".. FS mounted error");
	}

	// init screen
	tftBrigthnees();
	pTft->showFrame();
	
	// get actuel time 
	configTzTime(timezone, ntpServer);
	getLocalTime(&actualTime);
  pTft->showTime(actualTime, true);
	Serial.println("-- rtc sync with ntp");
 	
	initIrq();
	readConfigFile();		// read config data of clock
	
	pTft->showWeatherIcon(bild_44, pTft->getXPosWeatherNow(), pTft->getYPosWeatherNow());
	bGetWeather = true;
}

// ---------------------------------------------------------------------------------------------------
// loop
// ---------------------------------------------------------------------------------------------------
void loop(void) {
	static uint16_t u16StateRtcSync = 0;
	char strText[40];
	static char strTextOld[40] = " ";
	snprintf_P(strText, sizeof(strText), PSTR("%2.1f'C - %i%s - %ihPa"), tempToday, (int)humidityToday, "%", (int)pressureToday);

	if (pHMenue->getAktualMenue() != 0) {
		strTextOld[0] = 0;
	}

	pWifiServer->handleClient();
	
	pSw01->runState();
	pSw02->runState();

	pLed->SwPort(pSw01->Status());	// switch LED on with swith 1 - only for test

	tftBrigthnees();

	// sync rtc at 2:00
	switch (u16StateRtcSync) {
		case 0:
			if (actualTime.tm_hour == 2 && actualTime.tm_min == 0 && actualTime.tm_sec == 0) {
				configTzTime(timezone, ntpServer);
				Serial.println("-- rtc sync with ntp");
				u16StateRtcSync = 10;
			}
			break;
		case 10:
			if (actualTime.tm_hour == 3) {
				u16StateRtcSync = 0;
			}
			break;
		default:
			u16StateRtcSync = 0;
			break;
	}

	getLocalTime(&actualTime);
	clAlarm::Check();							// check alarm time
	
	pTft->showDateAndTime(actualTime, (const char*)WeekDay[actualTime.tm_wday]);	// line one to show date and time 		
	showAlarmTime(false);					
	pHMenue->handle();						// run menue
	
	// show clock and weather (bottom status line) only at menue number 0
	switch (pHMenue->getAktualMenue()) {
		case 0:		// runMainMenue
			if (strcmp(strTextOld, strText) != 0) { 	// check for new status text
				Serial.println(F("show status menue 0"));
				strcpy(strTextOld, strText);
				pTft->showState(strText);
			}
		case 1:		// changeAlarmTime
		case 2:		// changeAlarmTime
		case 5:		// runState
		case 6:		// runDeleteFile
			pTft->showTime(actualTime, false);
			
			if (bGetWeather) {
				getActualWeather();	
				pTft->showWeather(strIconToday.c_str(), pTft->getXPosWeatherNow(), pTft->getYPosWeatherNow());
				bGetWeather = false;
			}	
			break;
		case 3:		// runWeatherForcast
		case 4:		// runWeatherForcast
			break;
	}		   	
}

// ---------------------------------------------------------------------------------------------------
// menue functions
// ---------------------------------------------------------------------------------------------------
bool runMainMenue(void) {
	return clAlarm::enableAlarmTime(pSw02);
} 

bool changeAlarmTime(void) {
	static uint16_t u16Status = 0;
	static uint16_t u16StatusOld = 1;
		
	bool bResult = false;

	uint16_t _u16Nr;
	
	switch (pHMenue->getAktualMenue()) {
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
			if (!pSw02->Status()) {	
				u16Status = 5;
			}
			break;
		case 5:		// warte auf Tastendruck
			bResult = true;	
			if (pSw02->Status()) {	
				u16Status = 10;
			}
			break;
		case 10:
			if (_u16Nr < MAX_WECKER) {	
				if (Wecker[_u16Nr]->setNewAlarmTime()) {
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
			if (pSw02->Status()) {
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
			pTft->showVersion();	
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
			if (pSw02->Status()) {
				Serial.printf("delete file: %s\r\n", cFile);
				u16Status = 10;
			}
			break;
		case 10:
			if (SPIFFS.remove(cFile)) {
				pTft->showState("config. deleted");
			} else {
				pTft->showState("delete error");
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
			if (pSw02->Status()) {
				Serial.println("get weather forcast");
				pTft->showState("lade Wetterdaten");
				u16Status = 10;
			}
			break;
		case 10:
			getWeatherForcast();
			u16Status = 20;
			break;
		case 20:	
			if (!pSw02->Status()) {
				u32Timer = millis();
				u16Status = 30;
			}
			break;
		case 30:
			if (pSw02->Status()) {
				u16Status = 40;
			} else if (millis() > (u32Timer + (15 * 60000))) {
				u16Status = 40;
			}
			break;
		case 40:
			if (!pSw02->Status()) {
				pTft->clearMiddleArea(); 
				showAlarmTime(true);
				pTft->showTime(actualTime, true);
				bGetWeather = true;														// load weather icon
				pTft->showWeatherIcon(bild_44, pTft->getXPosWeatherNow(), pTft->getYPosWeatherNow());
				u16Status = 0;
			}
			break;
		default:
			u16Status = 0;
			break;
	}

	return bResult;
}

void showLabel(void) {
	char strText[40];
	char strIp[16];
	WiFi.localIP().toString().toCharArray(strIp, 16);
	snprintf_P(strText, sizeof(strText), PSTR("%s - %s"), WiFi.SSID().c_str(), strIp);
	pTft->showState(strText);
}

void showAlarmTime(bool _bForce) {
	static String oldString[MAX_WECKER];
	String strAlarmTime[MAX_WECKER];
	String strName;
	String strHour;
	String strMinute;
	String strDay;

	uint16_t xOffset = 64 + (2 * pTft->getHSpace());
	uint16_t yFontHeight = 21;
	uint16_t xFontWidth = 10;

	pTft->setFreeFont(pTft->DefaultFont);
	pTft->setTextSize(1);	
	pTft->setTextColor(TFT_YELLOW, TFT_BLACK);

	for (int i = 0; i < 2; i++) {
		uint16_t y = 141 + ((i * yFontHeight) + 2); 
	
		//   0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
		//   W 0 :   *   0 5 : 4 5   :        M  o     -  F  r
		strAlarmTime[i] = Wecker[i]->getTimeString(); 		
		strName   = strAlarmTime[i].substring(0,6);	 			// "W0: * "		
		strHour   = strAlarmTime[i].substring(6,8);				// "05"
		strMinute = strAlarmTime[i].substring(9,11);			// "45"
		strDay    = strAlarmTime[i].substring(14);				// "Mo"
		
		if ((oldString[i] != strAlarmTime[i]) || _bForce) {
			Serial.println(TraceTime() + strAlarmTime[i]);

			pTft->drawString(strName, xOffset + 10, y);						// name
			if (strName.substring(4,5) == " ") {
				pTft->fillRect(xOffset + (xFontWidth * 5), y, xFontWidth, yFontHeight, TFT_BLACK);
			}
			
			if (strHour.substring(0,1) != " ") {
				pTft->drawString(strHour, xOffset + (xFontWidth * 7), y);		// hour
			} else {
				pTft->fillRect(xOffset + (xFontWidth * 7), y, (xFontWidth * 3), yFontHeight, TFT_BLACK);
			}
			pTft->drawString(":", xOffset + 98, y);				// :
			if (strMinute.substring(0,1) != " ") {			
				pTft->drawString(strMinute, xOffset + (xFontWidth * 11), y);	// minute
			} else {
				pTft->fillRect(xOffset + (xFontWidth * 11), y, (xFontWidth * 3), yFontHeight, TFT_BLACK);
			}
			pTft->drawString(" - ", xOffset + 138, y);			// -
			if (strDay.substring(0,1) != " ") {
				pTft->drawString(strDay, xOffset + (xFontWidth * 16), y);		// day
			} else {
				pTft->fillRect(xOffset + (xFontWidth * 16), y, (xFontWidth * 7), yFontHeight, TFT_BLACK);
			}
			
			oldString[i] = strAlarmTime[i];
		}	
	}
}

// -------------------------------------------------------------------------------------------------
// TFT backligth brightness, controlled by LDR 
// -------------------------------------------------------------------------------------------------
void tftBrigthnees(void) {
	uint16_t pwmValue;
	uint16_t u16AnalogValue = analogRead(TFT_POTI);
	
	if (u16AnalogValue < 20) {
		u16AnalogValue = 20;
	}
	pwmValue = map(u16AnalogValue, 20, 900, 1, PWM_MAX);
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

	// reset authentication 
	if (getAuthentication()) {
		if (++authenticationTimer >= 60 * 5) {
			clearAuthentication();
		}
	} else {
		authenticationTimer = 0;
	}

	if (u32GetWeatherTimeout > 0) {u32GetWeatherTimeout--;}

	timer0_write(ESP.getCycleCount() + 80000000L); // for 80MHz this means 1 interrupt per seccond
}

// -----------------------------------------------------------------------------------
// init GPIO
// -----------------------------------------------------------------------------------
void initGpio(void) {
	Serial.println("-- init gpio");
	pSw01->Init(ParamSw01);
	pSw02->Init(ParamSw02);

	// buzzer
	pBuzzer->Init(BUZZER, POLARITY::NEG); // GIPO BUZZER low active
	pBuzzer->Off();

	// LED
	pLed->Init(LED, POLARITY::POS); 			// GPIO LED high active
	
	// init an switch on TFT backligth 
	analogWriteRange(PWM_MAX);
	analogWriteFreq(PWM_FREQ);
	analogWrite(TFT_BACKLIGHT, 1024); 	// valid value 0 - 1024
}

// -----------------------------------------------------------------------------------
// init display
// -----------------------------------------------------------------------------------
void initDisplay(void) {
	delay(1000);
	Serial.println("-- init dsplay");	
	pTft->init();
	pTft->setSwapBytes(true);				// used by push image function
	pTft->setRotation(ROTATION_90);
	pTft->fillScreen(TFT_BLACK);
	pTft->setTextColor(TFT_WHITE, TFT_BLACK);
}

// -----------------------------------------------------------------------------------
// init network
// -----------------------------------------------------------------------------------
void initNetwork(void) {
	pWifiManager->setAPCallback(wifiCallback);
	pWifiManager->setSaveConfigCallback(wifiCallbackSaveConfig);

	pTft->clearDisplay();
	pTft->setCursor(0, 30);
	pTft->setTextSize(1);	
	pTft->drawString(".. start WLan", 10, 10);

	if (pWifiManager->autoConnect("ESP_TFT_UHR")) {
		pTft->drawString(".. WLan connected", 10, 40);
		String strText = String(".. ") + WiFi.SSID() + String(" - ") + WiFi.localIP().toString();
		pTft->drawString(strText, 10, 70);

		MDNS.begin(cDnsName);

 		pWifiServer->on("/", handleIndex);
		pWifiServer->on("/values", HTTP_GET, handleValues);
	  pWifiServer->on("/config", handleConfig);
		pWifiServer->on("/delete", handleDelete);
		pWifiServer->on("/weather", handleWeather);
		pWifiServer->on("/Authentication", handleAuthentication);
		pWifiServer->on("/logout", handleLogout);
		pWifiServer->begin();
		pTft->drawString(".. server online", 10, 100);
		Serial.println(".. server online");
		delay(4000);
	} else {
		pTft->drawString(".. WLan error !!", 10, 40);
		pTft->drawString(".. ESP Reset !!", 10, 70);
		delay(3000);
		while (true) {;}
	}
}

void wifiCallback(WiFiManager *_pWiFiManager) {
	pTft->fillScreen(TFT_BLACK);
	pTft->println();
	pTft->println(".. Konfig. Mode aktiv");
	pTft->print(".. ");
	pTft->println(WiFi.softAPIP());
	pTft->println(String(".. ") + _pWiFiManager->getConfigPortalSSID());
}

// callback notifying us of the need to save config
void wifiCallbackSaveConfig () {
	Serial.println(F("wifi data saved"));
}

// -----------------------------------------------------------------------------------
// init file system and read configuration
// -----------------------------------------------------------------------------------
void readConfigFile(void) {
	if (SPIFFS.exists("/config.json")) {
		// file exists, reading and loading
		Serial.println(".. open config file");
		File configFile = SPIFFS.open("/config.json", "r");
		if (configFile) {
			Serial.println(".. config file read");
			size_t size = configFile.size();
			
			// allocate a buffer to store contents of the file.
			std::unique_ptr<char[]> buf(new char[size]);

			configFile.readBytes(buf.get(), size);

			DynamicJsonDocument json(1024);
			auto deserializeError = deserializeJson(json, buf.get());
			serializeJson(json, Serial);
			
			if (!deserializeError) {
				Serial.println(F("\nparsed json"));
				char strData[20];  
				char strMaxWecker[5];

				snprintf_P(strData, sizeof(strData), PSTR("MaxWecker"));
				strcpy(strMaxWecker, json[strData]);
				String Data = (String)strMaxWecker;
				int iMaxWecker = Data.toInt();
				
				for (int i = 0; i < iMaxWecker; i++) {
					if (Wecker[i] != nullptr) {
						snprintf_P(strData, sizeof(strData), PSTR("WeckerStunden_%i") , i);
						Wecker[i]->setNewAlarmHour(json[strData]);
												
						snprintf_P(strData, sizeof(strData), PSTR("WeckerMinuten_%i") , i);
						Wecker[i]->setNewAlarmMinute(json[strData]);
						
						snprintf_P(strData, sizeof(strData), PSTR("WeckerTage_%i") , i);
						Wecker[i]->setNewWeekDay(json[strData]);
						
						snprintf_P(strData, sizeof(strData), PSTR("WeckerAktiv_%i") , i);
						if (json[strData] == String('*')) {
							Serial.println((String)".. Wecker " + String(i) + (String)" ist aktiv");
							Wecker[i]->Start();
						} else {
							Wecker[i]->Stop();
						}
					}	else {
							Serial.println((String)".. fail to read config data for alarm " + String(i));
							return;
					}
				}					
				Serial.print(F("memory used : "));
				Serial.println(json.memoryUsage());
			} else {
				Serial.println(".. failed to load json config");
			}
			configFile.close();
		}
  	} else {
    	Serial.println(".. failed to open config file");
  	}
}

void saveWeckerConfig(void) {
	Serial.println(F("alarm clock configuration saved"));
	DynamicJsonDocument json(1024);
	char strData[20]; 
	
	snprintf_P(strData, sizeof(strData), PSTR("MaxWecker"));
	json[strData] = (String)MAX_WECKER;

	for(int i = 0; i < MAX_WECKER; i++) {
		snprintf_P(strData, sizeof(strData), PSTR("WeckerStunden_%i") , i);
		json[strData] = Wecker[i]->getAlarmHour(); 
		
		snprintf_P(strData, sizeof(strData), PSTR("WeckerMinuten_%i") , i);
		json[strData] = Wecker[i]->getAlarmMinute();
		
		snprintf_P(strData, sizeof(strData), PSTR("WeckerTage_%i") , i);
		json[strData] = Wecker[i]->getAlarmDay(); 
		
		snprintf_P(strData, sizeof(strData), PSTR("WeckerAktiv_%i") , i);
		if (Wecker[i]->getStatus()) {
			json[strData] = "*";  
		} else {
			json[strData] = " ";  
		}
	}

	File configFile = SPIFFS.open("/config.json", "w");
	Serial.println();

	if (configFile) {
		serializeJson(json, Serial);
		serializeJson(json, configFile);
		configFile.close();
		Serial.println(F("data saved"));

		pBuzzer->On();
		delay(10);
		pBuzzer->Off();
	} else { 
		Serial.println(F("failed to open config file for writing"));
	}
}

// -----------------------------------------------------------------------------------
// read data from web page
// -----------------------------------------------------------------------------------
String getJsonDataFromWeb (String _Server, String _Url) {
	String stWeatherString;
	u32GetWeatherTimeout = 5;		// set timeout to xs

	if (pWifiClient != nullptr) {
		
		Serial.println(TraceTime() + String("server ") + _Server);
		Serial.println(TraceTime() + String("url    ") + _Url);

		while (!pWifiClient->connect(_Server, 80)) {
			delay(500);
			if (u32GetWeatherTimeout == 0) {goto error;}
		}
		
		Serial.println(TraceTime() + String("connect to ") + Server);
		pWifiClient->println(_Url);

		while (!pWifiClient->available()) {
			if (u32GetWeatherTimeout == 0) {goto error;}
		}

		Serial.println(TraceTime() + String("get data"));
		stWeatherString = pWifiClient->readString();
		Serial.println(TraceTime() + String("data received"));
		return stWeatherString;
	}
error:
	Serial.println(TraceTime() + String("**** connection error"));
	return String(F("error"));
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
	if (_WetterDaten == String("error")) {
		Serial.println("**** error get weather data");
		return;
	}

	Serial.println(TraceTime() + "decodeCurrentWeather");
	DynamicJsonDocument jsonWeatherToday(900);
		
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
	
	if (pHMenue->getAktualMenue() == 3) {
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
		pTft->showState(error.c_str());
		return;
	} else {
		// get data from JSON-tree 
		u16Count = jsonWeatherForecast["cnt"];
		Serial.println(String("Wettervorhersage für ") + u16Count + String (" Tage"));
		
		if (u16Count > FORECAST) {
			u16Count = FORECAST;
		}
	
		for (int i = 0; i < u16Count; i++) {
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
			strIconForecast[i]    = jsonWeatherForecast["list"][i]["weather"][0]["icon"].as<String>();

			strcpy(&cDay[i][0], WeekDay[(actualTime.tm_wday + i) % 7]);
			
			Serial.println(F("----------------------------------------------"));
			snprintf_P(strData, sizeof(strData), PSTR("Datum        : %s %02d.%02d"), WeekDay[actualTime.tm_wday], actualTime.tm_mday, actualTime.tm_mon + 1);
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
		
		pTft->clearMiddleArea();
		
		for (int i = 0; i < u16Count; i++) {
			yield();
			pTft->drawCentreString(String(cDay[i]), 8 + ((i * 80) + 32), pTft->getYMiddle() + 8, 1);
			pTft->showWeather(strIconForecast[i].c_str(), 8 + (i * 80), pTft->getYMiddle() + 30);
			pTft->drawCentreString(String(tempDayForecast[i], 0) + String("'C"), 8 + ((i * 80) + 32), pTft->getYMiddle() + 8 + 64 + 24, 1);
			pTft->drawCentreString(String(humidityForecast[i], 0) + String("%"), 8 + ((i * 80) + 32), pTft->getYMiddle() + 8 + 64 + 48, 1);
		}
		
		Serial.print(F("memory used : "));
		Serial.println(jsonWeatherForecast.memoryUsage());

		pTft->showState(convertStringToGerman(String("Wetter in ") + strCityNameForecast).c_str());
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
}

ICACHE_RAM_ATTR void irqSw02(void) {
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

