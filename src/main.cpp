#include <Adafruit_GFX.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_ILI9341.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Digital_7_V448pt7b.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266_Heweather.h>
#include <Fonts\FreeMonoBold9pt7b.h>
#include <SPI.h>
#include <WeatherFont20pt7b.h>
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
struct tm *timeInfo, lastTimeInfo;
const String WDAY_NAMES[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const String MONTH_NAMES[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
String lastDateStr, lastTimeStr;

//定时任务
FTask TaskNTP, TaskDrawWeather, TaskDrawDateTime, TaskDrawTestRuler;

struct rect {
	int16_t x, y;
	uint16_t w, h;
} recta, rectb;

/**********方法************/
void DrawWeather();
void DrawDate();
void DrawTime();
void DrawDateTime();
void UpdateNTP();
void PrintTime();
void SyncTime();
void tftSet(const GFXfont *, uint16_t, uint8_t);
void getTFTTextDifRect(String, String, int16_t, int16_t);
void TestDrawRuler();

/*********入口************/
void initSetial() {	 //初始化串口
	Serial.begin(9600);
	Serial.println("\n\n[WeatherDesktopNTP_ILI9341]\n");
}
void initTFT() {  //初始化TFT屏幕
	tft.begin(800000000ul);
	tft.setRotation(1);
	tft.fillScreen(ILI9341_BLACK);
	yield();
	tftSet(&FreeMonoBold9pt7b, ILI9341_WHITE, 1);
}
void initWifi() {  //初始化Wifi
	WiFi.mode(WIFI_STA);
	WiFi.begin(SSID, PSWD);
	tft.setCursor(0, 100);
	tft.printf("Connet to %s\n", SSID);
	Serial.printf("Connet to %s\n", SSID);
	yield();
	while (!WiFi.isConnected()) {
		tft.print(".");
		Serial.print(".");
		delay(500);
		yield();
	}
	tft.println("OK");
	Serial.println("OK");
	tft.fillScreen(ILI9341_BLACK);
	yield();
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
	UpdateNTP();
	UpdateNTP();
}

void loop() {
	TaskNTP.run();
	TaskDrawDateTime.run();
	TaskDrawWeather.run();
	TaskDrawTestRuler.run();
}

void DrawWeather() {  //绘制天气信息
	//tft.fillScreen(ILI9341_BLACK);
	tft.fillRect(0, 200, 320, 80, ILI9341_BLACK);
	yield();
	tft.setCursor(0, 200);

	if (weatherNow.get()) {	 // 获取天气更新
		tftSet(&FreeMonoBold9pt7b, ILI9341_WHITE, 1);

		tft.printf("Weather: %s\n", weatherNow.getWeatherText().c_str());
		tft.printf("Temp: %d(%d)", weatherNow.getTemp(), weatherNow.getFeelLike());
		tft.setCursor(tft.width() - 14 * 8, tft.getCursorY());
		tft.printf("Hum: %.1f\n", weatherNow.getHumidity());
		tft.printf("Wind: %s %d", weatherNow.getWindDir().c_str(), weatherNow.getWindScale());
		tft.setCursor(tft.width() - 14 * 8, tft.getCursorY());
		tft.printf("Rain: %.1f\n", weatherNow.getPrecip());

		//tft.printf("Icon: %d\n", weatherNow.getIcon());
		//tft.printf("Update: %s\n", weatherNow.getLastUpdate().substring(0, 16).c_str());
	} else {
		tft.println("Update Failed...");
	}
}
void DrawDate() {  //绘制日期
	tftSet(&FreeMonoBold9pt7b, ILI9341_WHITE, 1);

	char aaa[64];
	snprintf(aaa, sizeof(aaa), "%s %s %02d %4d",
			 WDAY_NAMES[timeInfo->tm_wday].c_str(),
			 MONTH_NAMES[timeInfo->tm_mon].c_str(),
			 timeInfo->tm_mday,
			 timeInfo->tm_year + 1900);
	String bb = String(aaa);
	Serial.println(bb);
	getTFTTextDifRect(bb, lastDateStr, 1, 16);
	lastDateStr = bb;
}
void DrawTime() {  //绘制时间
	tftSet(&Digital_7_V448pt7b, ILI9341_WHITE, 1);

	char aaa[64];
	snprintf(aaa, sizeof(aaa), "%02d:%02d:%02d",
			 timeInfo->tm_hour,
			 timeInfo->tm_min,
			 timeInfo->tm_sec);
	String bb = String(aaa);
	Serial.println(bb);
	getTFTTextDifRect(bb, lastTimeStr, 20, 130);
	lastTimeStr = bb;

}
void UpdateNTP() {	//网络同步时间
#define TZ 8		// (utc+) TZ in hours
#define DST_MN 0	// use 60mn for summer time in some countries
#define TZ_MN ((TZ)*60)
#define TZ_SEC ((TZ)*3600)
#define DST_SEC ((DST_MN)*60)
	configTime(TZ_SEC, DST_SEC, "pool.ntp.org", "0.cn.pool.ntp.org", "1.cn.pool.ntp.org");
	yield();
	SyncTime();
	DrawDate();
	Serial.println("NTP sync!");
}
void SyncTime() {  //获取本地时间
	now = time(nullptr);
	timeInfo = localtime(&now);
}
void DrawDateTime() {  //绘制时间和日期
	SyncTime();
	DrawTime();
	if (timeInfo->tm_hour == 0 && timeInfo->tm_min == 0 && timeInfo->tm_sec == 0) {
		DrawDate();
	}
}
// void PrintTime() {	//打印时间
// 	Serial.printf("%s %s %02d %4d %02d:%02d:%02d \n",
// 				  WDAY_NAMES[timeInfo.tm_wday].c_str(),
// 				  MONTH_NAMES[timeInfo.tm_mon].c_str(),
// 				  timeInfo.tm_mday,
// 				  timeInfo.tm_year + 1900,
// 				  timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
// }
void tftSet(const GFXfont *font, uint16_t color, uint8_t size) {  //TFT 设置字体/颜色/大小
	tft.setFont(font);
	tft.setTextColor(color);
	tft.setTextSize(size);
}
void TestDrawRuler() {	//测试_绘制点阵尺
	return;
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
void getTFTTextDifRect(String news, String olds, int16_t cx, int16_t cy) {	//差异化更新文本
	int a, l = min(news.length(), olds.length());
	for (int i = 0; i < l; i++) {
		if (news[i] != olds[i]) {
			a = i;
			break;
		}
	}
	String b = olds.substring(0, a);
	tft.getTextBounds(b, cx, cy, &recta.x, &recta.y, &recta.w, &recta.h);
	tft.getTextBounds(olds, cx, cy, &rectb.x, &rectb.y, &rectb.w, &rectb.h);
	recta.x += recta.w;
	//tft.drawRect(recta.x, recta.y, rectb.w - recta.w, rectb.h, ILI9341_RED);
	tft.setCursor(recta.x, cy);
	tft.setTextColor(ILI9341_BLACK);
	tft.print(olds.substring(a, olds.length()));
	tft.setCursor(recta.x, cy);
	tft.setTextColor(ILI9341_WHITE);
	tft.print(news.substring(a, news.length()));
}