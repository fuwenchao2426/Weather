#include <Adafruit_GFX.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_ILI9341.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266_Heweather.h>
#include <Fonts/Org_01.h>
#include <Fonts\FreeMonoBold18pt7b.h>
#include <Fonts\FreeMonoBold9pt7b.h>
#include <SPI.h>
#include <lasttime.h>
#include <sys/time.h>
#include <time.h>

#define TFT_DC D4
#define TFT_CS D8

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
WeatherNow weatherNow;

#define SSID "SuperliiiGroup"
#define PSWD "meimima_1"

//和风天气 实时天气API 10~20分钟更新一次
String UserKey = "f9edcd676f79449e89d6479b4d89469c";  // 私钥 https://dev.heweather.com/docs/start/get-api-key
String Location = "101070211";						  // 城市代码 https://github.com/heweather/LocationList,表中的 Location_ID 大连甘井子区 101070211
String Unit = "m";									  // 公制-m/英制-i
String Lang = "en";									  // 语言 英文-en/中文-zh

//时间
time_t now;
struct tm* timeInfo;
const String WDAY_NAMES[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const String MONTH_NAMES[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

//定时任务
FTask TaskNTP, TaskDrawWeather, TaskDrawDateTime, TaskDrawTestRuler;

/**********方法************/
void DrawWeather();
void DrawDate();
void DrawTime();
void DrawDateTime();
void UpdateNTP();
void PrintTime();
void SyncTime();
void tftSet(const GFXfont*, uint16_t, uint8_t);
void TestDrawRuler();

/*********入口************/
void initSetial() {	 //初始化串口
	Serial.begin(9600);
	Serial.println("\n\n[WeatherDesktopNTP_ILI9341]\n");
}
void initTFT() {  //初始化TFT屏幕
	tft.begin(80000000);
	tft.setRotation(1);
	tft.fillScreen(ILI9341_BLACK);
	tft.setFont(&FreeMonoBold9pt7b);
	tft.setTextColor(ILI9341_WHITE);
	tft.setTextSize(1);
}
void initWifi() {  //初始化Wifi
	WiFi.mode(WIFI_STA);
	WiFi.begin(SSID, PSWD);
	tft.setCursor(0, 20);
	tft.printf("Connet to %s\n", SSID);
	Serial.printf("Connet to %s\n", SSID);
	yield();
	while (!WiFi.isConnected()) {
		tft.print(".");
		Serial.print(".");
		delay(500);
		yield();
	}
	tft.println();
	Serial.println();
}

void setup() {
	initSetial();
	initTFT();
	initWifi();

	weatherNow.config(UserKey, Location, Unit, Lang);

	TaskNTP.initM(UpdateNTP, 5);
	TaskDrawDateTime.initS(DrawDateTime, 1);
	TaskDrawWeather.initM(DrawWeather, 1.5);
	TaskDrawTestRuler.initS(TestDrawRuler, 5);
}

void loop() {
	TaskNTP.run();
	TaskDrawDateTime.run();
	TaskDrawWeather.run();
	TaskDrawTestRuler.run();
}

void DrawWeather() {  //绘制天气信息
	tft.fillScreen(ILI9341_BLACK);
	tft.setCursor(0, 160);

	if (weatherNow.get()) {	 // 获取天气更新
		tftSet(&FreeMonoBold9pt7b, ILI9341_WHITE, 1);

		//tft.printf("\tUpdate: %s\n", weatherNow.getLastUpdate().substring(0, 16).c_str());
		tft.printf("\tTemp: %d(%d)", weatherNow.getTemp(), weatherNow.getFeelLike());
		tft.setCursor(tft.width() - 14 * 8, tft.getCursorY());
		tft.printf("Hum: %.1f\n", weatherNow.getHumidity());
		//tft.printf("\tIcon: %d\n", weatherNow.getIcon());
		tft.printf("\tWeather: %s\n", weatherNow.getWeatherText().c_str());
		tft.printf("\twindDir: %s %d\n", weatherNow.getWindDir().c_str(), weatherNow.getWindScale());
		tft.printf("\tPrecip: %.1f\n", weatherNow.getPrecip());

	} else {
		tft.println("Update Failed...");
	}
}
void DrawDate() {  //绘制日期
	tft.fillRect(0, 0, 170, 10, ILI9341_BLACK);
	tftSet(&FreeMonoBold9pt7b, ILI9341_WHITE, 1);

	tft.setCursor(1, 16);
	tft.printf("%s %s %02d %4d",
			   WDAY_NAMES[timeInfo->tm_wday].c_str(),
			   MONTH_NAMES[timeInfo->tm_mon].c_str(),
			   timeInfo->tm_mday,
			   timeInfo->tm_year + 1900);
}
void DrawTime() {  //绘制时间
	tft.fillRect(80, 40, 170, 15, ILI9341_BLACK);
	tftSet(&FreeMonoBold18pt7b, ILI9341_WHITE, 1);

	tft.setCursor(80, 64);
	tft.printf("%02d:%02d:%02d",
			   timeInfo->tm_hour,
			   timeInfo->tm_min,
			   timeInfo->tm_sec);
}
void UpdateNTP() {	//网络同步时间
#define TZ 8		// (utc+) TZ in hours
#define DST_MN 0	// use 60mn for summer time in some countries
#define TZ_MN ((TZ)*60)
#define TZ_SEC ((TZ)*3600)
#define DST_SEC ((DST_MN)*60)
	configTime(TZ_SEC, DST_SEC, "pool.ntp.org", "0.cn.pool.ntp.org", "1.cn.pool.ntp.org");
	Serial.println("NTP sync!");
}
void SyncTime() {  //获取本地时间
	now = time(nullptr);
	timeInfo = localtime(&now);
}
void DrawDateTime() {  //绘制时间和日期
	SyncTime();
	DrawTime();
	DrawDate();
}
void PrintTime() {	//打印时间
	now = time(nullptr);
	struct tm* timeInfo;
	timeInfo = localtime(&now);

	Serial.printf("%s %s %02d %4d %02d:%02d:%02d \n",
				  WDAY_NAMES[timeInfo->tm_wday].c_str(),
				  MONTH_NAMES[timeInfo->tm_mon].c_str(),
				  timeInfo->tm_mday,
				  timeInfo->tm_year + 1900,
				  timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
}
void tftSet(const GFXfont* font, uint16_t color, uint8_t size) {  //TFT 设置字体/颜色/大小
	tft.setFont(font);
	tft.setTextColor(color);
	tft.setTextSize(size);
}
void TestDrawRuler() {	//测试_绘制点阵尺
	for (int x = 0; x < tft.width(); x += 10) {
		for (int y = 0; y < tft.height(); y += 10) {
			tft.drawPixel(x, y, ILI9341_YELLOW);
			if (x / 10 % 5 == 0 && y / 10 % 5 == 0) {
				tft.drawPixel(x + 1, y, ILI9341_YELLOW);
				tft.drawPixel(x - 1, y, ILI9341_YELLOW);
				tft.drawPixel(x, y + 1, ILI9341_YELLOW);
				tft.drawPixel(x, y - 1, ILI9341_YELLOW);
			}
		}
	}
}