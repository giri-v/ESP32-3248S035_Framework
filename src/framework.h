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
#include <JPEGDecoder.h>
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

#define minimum(a, b) (((a) < (b)) ? (a) : (b))
#define maximum(a, b) (((a) > (b)) ? (a) : (b))

#include <AudioFileSourceSD.h>
#include <AudioFileSourceID3.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

AudioGeneratorMP3 *mp3;
AudioOutputI2S *out;
bool mp3Done = true;

TFT_eSPI tft = TFT_eSPI(); // Create object "tft"
OpenFontRender ofr;
int screenWidth = tft.width();
int screenHeight = tft.height();
int screenCenterX = tft.width() / 2;
int screenCenterY = tft.height() / 2;
PNG png;
int32_t xPos = 0;
int32_t yPos = 0;


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

// PNGDec Veriables
File pngfile;



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

// PNGDec File I/O Helper Functions
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

/// @brief Draw the PNG Image 
/// @param pDraw 
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

// ####################################################################################################
//  Draw a JPEG on the TFT, images will be cropped on the right/bottom sides if they do not fit
// ####################################################################################################
//  This function assumes xpos,ypos is a valid screen coordinate. For convenience images that do not
//  fit totally on the screen are cropped to the nearest MCU size and may leave right/bottom borders.
void renderJPEG(int xpos, int ypos)
{

    // retrieve information about the image
    uint16_t *pImg;
    uint16_t mcu_w = JpegDec.MCUWidth;
    uint16_t mcu_h = JpegDec.MCUHeight;
    uint32_t max_x = JpegDec.width;
    uint32_t max_y = JpegDec.height;

    // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
    // Typically these MCUs are 16x16 pixel blocks
    // Determine the width and height of the right and bottom edge image blocks
    uint32_t min_w = minimum(mcu_w, max_x % mcu_w);
    uint32_t min_h = minimum(mcu_h, max_y % mcu_h);

    // save the current image block size
    uint32_t win_w = mcu_w;
    uint32_t win_h = mcu_h;

    // record the current time so we can measure how long it takes to draw an image
    uint32_t drawTime = millis();

    // save the coordinate of the right and bottom edges to assist image cropping
    // to the screen size
    max_x += xpos;
    max_y += ypos;

    // read each MCU block until there are no more
    while (JpegDec.read())
    {

        // save a pointer to the image block
        pImg = JpegDec.pImage;

        // calculate where the image block should be drawn on the screen
        int mcu_x = JpegDec.MCUx * mcu_w + xpos; // Calculate coordinates of top left corner of current MCU
        int mcu_y = JpegDec.MCUy * mcu_h + ypos;

        // check if the image block size needs to be changed for the right edge
        if (mcu_x + mcu_w <= max_x)
            win_w = mcu_w;
        else
            win_w = min_w;

        // check if the image block size needs to be changed for the bottom edge
        if (mcu_y + mcu_h <= max_y)
            win_h = mcu_h;
        else
            win_h = min_h;

        // copy pixels into a contiguous block
        if (win_w != mcu_w)
        {
            uint16_t *cImg;
            int p = 0;
            cImg = pImg + win_w;
            for (int h = 1; h < win_h; h++)
            {
                p += mcu_w;
                for (int w = 0; w < win_w; w++)
                {
                    *cImg = *(pImg + w + p);
                    cImg++;
                }
            }
        }

        // calculate how many pixels must be drawn
        uint32_t mcu_pixels = win_w * win_h;

        tft.startWrite();

        // draw image MCU block only if it will fit on the screen
        if ((mcu_x + win_w) <= tft.width() && (mcu_y + win_h) <= tft.height())
        {

            // Now set a MCU bounding window on the TFT to push pixels into (x, y, x + width - 1, y + height - 1)
            tft.setAddrWindow(mcu_x, mcu_y, win_w, win_h);

            // Write all MCU pixels to the TFT window
            while (mcu_pixels--)
            {
                // Push each pixel to the TFT MCU area
                tft.pushColor(*pImg++);
            }
        }
        else if ((mcu_y + win_h) >= tft.height())
            JpegDec.abort(); // Image has run off bottom of screen so abort decoding

        tft.endWrite();
    }

    // calculate how long it took to draw the image
    drawTime = millis() - drawTime;

    // print the results to the serial port
    // Log.infoln("renderJPEG(): Total render time was: %ims", drawTime);
}

// ####################################################################################################
//  Print image information to the serial port (optional)
// ####################################################################################################
void jpegInfo()
{
    Log.infoln(F("==============="));
    Log.infoln(F("JPEG image info"));
    Log.infoln(F("==============="));
    Log.info(F("Width      :"));
    Log.infoln("%i", JpegDec.width);
    Log.info(F("Height     :"));
    Log.infoln("%i", JpegDec.height);
    Log.info(F("Components :"));
    Log.infoln("%i", JpegDec.comps);
    Log.info(F("MCU / row  :"));
    Log.infoln("%i", JpegDec.MCUSPerRow);
    Log.info(F("MCU / col  :"));
    Log.infoln("%i", JpegDec.MCUSPerCol);
    // Log.info(F("Scan type  :"));
    // Log.infoln(JpegDec.scanType);
    Log.info(F("MCU width  :"));
    Log.infoln("%i", JpegDec.MCUWidth);
    Log.info(F("MCU height :"));
    Log.infoln("%i", JpegDec.MCUHeight);
    Log.infoln(F("==============="));
}

// ####################################################################################################
//  Show the execution time (optional)
// ####################################################################################################
//  WARNING: for UNO/AVR legacy reasons printing text to the screen with the Mega might not work for
//  sketch sizes greater than ~70KBytes because 16-bit address pointers are used in some libraries.

// The Due will work fine with the HX8357_Due library.

void showTime(uint32_t msTime)
{
    // tft.setCursor(0, 0);
    // tft.setTextFont(1);
    // tft.setTextSize(2);
    // tft.setTextColor(TFT_WHITE, TFT_BLACK);
    // tft.print(F(" JPEG drawn in "));
    // tft.print(msTime);
    // tft.println(F(" ms "));
    Log.info(F(" JPEG drawn in "));
    Log.info("%i", msTime);
    Log.infoln(F(" ms "));
}

void drawArrayJpeg(const uint8_t arrayname[], uint32_t array_size, int xpos, int ypos)
{

    boolean decoded = JpegDec.decodeArray(arrayname, array_size);

    if (decoded)
    {
        // print information about the image to the serial port
        jpegInfo();

        // render the image onto the screen at given coordinates
        renderJPEG(xpos, ypos);
    }
    else
    {
        Serial.println("Jpeg file format not supported!");
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