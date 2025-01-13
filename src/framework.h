#ifndef FRAMEWORK_H
#define FRAMEWORK_H

#include <Arduino.h>
#include <time.h>

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
}

#ifndef SECRETS_H
#define SECRETS_H

#define HOSTNAME "myESP8266";
#define NTP_SERVER "pool.ntp.org"

#define WIFI_SSID "APName"
#define WIFI_PASSWORD "APPassword"

#define HTTP_SERVER "192.168.0.12"
#define HTTP_PORT 5000

#define MQTT_HOST IPAddress(192, 168, 0, 200)
#define MQTT_PORT 1883

#define LATITUDE 37.3380937
#define LONGITUDE -121.8853892

#endif // SECRETS_H
#include <WiFi.h>
#include <Preferences.h>
#include <SPI.h>
#include <SD.h>

#ifndef LittleFS
#include <SPIFFS.h>
#else
#include <LittleFS.h>
#endif
#include <Update.h>

#define SD_CS 5

#include <TFT_eSPI.h>
#include <OpenFontRender.h>
#include <PNGdec.h>
#define MAX_IMAGE_WIDTH 320

#include <HTTPClient.h>
#include <AsyncMqttClient.h>
#include <ArduinoJson.h>

#include <ArduinoLog.h>

#include <TLogPlus.h>

// using namespace TLogPlus;

#ifdef TELNET_LOGGING
#include <TelnetSerialStream.h>
using namespace TLogPlusStream;
TelnetSerialStream telnetSerialStream = TelnetSerialStream();
#endif

#ifdef WEBSTREAM_LOGGING
#include <WebSerialStream.h>
using namespace TLogPlusStream;
WebSerialStream webSerialStream = WebSerialStream();
#endif

#ifdef SYSLOG_LOGGING
#include <SyslogStream.h>
using namespace TLogPlusStream;
SyslogStream syslogStream = SyslogStream();
#endif

#ifdef MQTT_LOGGING
#include <MqttlogStream.h>
#include "main.h"
using namespace TLogPlusStream;
// EthernetClient client;
WiFiClient client;
MqttStream mqttStream = MqttStream(&client);
char topic[128] = "log/foo";
#endif

#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

AudioGeneratorMP3 *mp3;
AudioOutputI2S *out;
bool mp3Done = true;

TFT_eSPI tft = TFT_eSPI(); // Create object "tft"
OpenFontRender ofr;
int screenWidth = tft.width();
int screenHeight = tft.height();
int screenCenterX = tft.width() / 2;
int screenCenterY = tft.height() / 2;

int chip_id = ESP.getEfuseMac();

#ifdef APP_NAME
const char *appName = APP_NAME;
#endif


int appInstanceID = -1;
char friendlyName[100] = "NoNameSet";

bool isFirstLoop = true;
bool isGoodTime = false;
bool isFirstDraw = true;

#ifdef NTP_SERVER
const char *ntpServer = NTP_SERVER;
#endif

// ********** Connectivity Parameters **********
AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;
TimerHandle_t checkFWUpdateTimer;
TimerHandle_t appInstanceIDWaitTimer;
TimerHandle_t wifiFailCountTimer;

int wifiFailCount = 0;

int volume = 50; // Volume is %
int bootCount = 0;
esp_sleep_wakeup_cause_t wakeup_reason;
esp_reset_reason_t reset_reason;
int maxOtherIndex = -1;

Preferences preferences;

#ifdef HOSTNAME
String hostname = HOSTNAME;
#endif

uint8_t macAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};




// MQTT Topics (25 character limit per level)
char onlineTopic[100];
char willTopic[100];
char appSubTopic[100];

char latestFirmwareFileName[100];

// **************** Debug Parameters ************************
String methodName = "";

// Here are the callback functions that the decPNG library
// will use to open files, fetch data and close the file.

File pngfile;

void *pngOpen(const char *filename, int32_t *size)
{
    Log.verboseln("Attempting to open %s\n", filename);
    pngfile = SPIFFS.open(filename, "r");
    *size = pngfile.size();
    return &pngfile;
}

void pngClose(void *handle)
{
    File pngfile = *((File *)handle);
    if (pngfile)
        pngfile.close();
}

int32_t pngRead(PNGFILE *page, uint8_t *buffer, int32_t length)
{
    if (!pngfile)
        return 0;
    page = page; // Avoid warning
    return pngfile.read(buffer, length);
}

int32_t pngSeek(PNGFILE *page, int32_t position)
{
    if (!pngfile)
        return 0;
    page = page; // Avoid warning
    return pngfile.seek(position);
}

bool isNullorEmpty(char *str)
{
    if ((str == NULL) || (str[0] == '\0'))
        return true;
    else
        return false;
}

bool isNullorEmpty(String str)
{
    return isNullorEmpty(str.c_str());
}

// check a string to see if it is numeric
bool isNumeric(char *str)
{
    for (byte i = 0; str[i]; i++)
    {
        if (!isDigit(str[i]))
            return false;
    }
    return true;
}

void initAudioOutput()
{
    out = new AudioOutputI2S(0, 2, 8, -1); // Output to builtInDAC
    out->SetOutputModeMono(true);
    out->SetGain(1.0);
}


void playMP3(char *filename)
{
    AudioFileSourceSD *file;
    AudioFileSourceID3 *id3;

    file = new AudioFileSourceSD(filename);
    id3 = new AudioFileSourceID3(file);
    mp3 = new AudioGeneratorMP3();
    if (!mp3->begin(id3, out))
    {
        Log.errorln("Failed to begin MP3 decode.");
    }
    else
    {
        mp3Done = false;
    }
}

void playMP3Loop()
{
    if (mp3->isRunning())
    {
        if (!mp3->loop())
        {
            mp3->stop();
            mp3Done = true;
        }
    }
    else
    {
        mp3Done = true;
    }
}

void pngDraw(PNGDRAW *pDraw)
{
    uint16_t lineBuffer[MAX_IMAGE_WIDTH];
    static uint16_t dmaBuffer[MAX_IMAGE_WIDTH]; // static so buffer persists after fn exit

    png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
    tft.pushImage(xPos, yPos + pDraw->y, pDraw->iWidth, 1, lineBuffer);
}

void drawPNG(const char *filename, int x, int y)
{

    int16_t rc = png.open(filename, pngOpen, pngClose, pngRead, pngSeek, pngDraw);
    xPos = x;
    yPos = y;
    if (rc == PNG_SUCCESS)
    {
        tft.startWrite();
        Log.verboseln("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
        uint32_t dt = millis();
        if (png.getWidth() > MAX_IMAGE_WIDTH)
        {
            Serial.println("Image too wide for allocated lin buffer!");
        }
        else
        {
            rc = png.decode(NULL, 0);
            png.close();
        }
        tft.endWrite();
    }
}

void clearScreen()
{
    tft.fillScreen(TFT_BLACK);
}

void drawString(String text, int x, int y)
{
    ofr.setCursor(x, y);
    if (ofr.getAlignment() == Align::MiddleCenter)
    {
        ofr.setCursor(x, y - ofr.getFontSize() / 5 - 2);
    }
    ofr.printf(text.c_str());
}

void drawString(String text, int x, int y, int font_size)
{
    ofr.setFontSize(font_size);
    drawString(text, x, y);
}

void drawString(String text, int x, int y, int font_size, int color)
{
    ofr.setFontColor(color);
    drawString(text, x, y, font_size);
}

void drawString(String text, int x, int y, int font_size, int color, int bg_color)
{
    ofr.setFontColor(color, bg_color);
    drawString(text, x, y, font_size);
}

int webGet(String req, String &res)
{
    String oldMethodName = methodName;
    methodName = "webGet(String req, String &res)";

    int result = -1;

    Log.verboseln("Connecting to http://%s:%d%s", HTTP_SERVER, HTTP_PORT,
                  req.c_str());

    WiFiClient client;
    HTTPClient http;
    String payload;
    Log.verboseln("[HTTP] begin...");

    // int connRes = client.connect(IPAddress(192,168,0,12), 5000);
    // Log.verboseln("Connected: %d", connRes);

    // if (http.begin(client, req))
    if (http.begin(client, HTTP_SERVER, HTTP_PORT, req))
    { // HTTP

        Log.verboseln("[HTTP] GET...");
        // start connection and send HTTP header
        int httpCode = http.GET();
        result = httpCode;

        // httpCode will be negative on error
        if (httpCode > 0)
        {
            // HTTP header has been send and Server response header has
            // been handled
            Log.verboseln("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
            {
                res = http.getString();
            }
        }
        else
        {
            Log.errorln("[HTTP] GET... failed, error: %s", http.errorToString(httpCode).c_str());
        }

        http.end();
    }
    else
    {
        Log.warningln("[HTTP] Unable to connect");
    }

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
    return result;
}
#endif // FRAMEWORK_H