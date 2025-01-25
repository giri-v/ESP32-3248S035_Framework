////////////////////////////////////////////////////////////////////
/// @file app_functions.h
/// @brief Contains application specific includes. variables and functions
////////////////////////////////////////////////////////////////////

#ifndef APP_FUNCTIONS_H
#define APP_FUNCTIONS_H

#include "framework.h"

#ifdef USE_WEB_SERVER
#include "web_server.h"
#endif

#include "../assets/fonts/Roboto.h"

// ********* Framework App Parameters *****************




bool isFirstDraw = true;

// ********** Connectivity Parameters **********



// ********** App Global Variables **********

////////////////////////////////////////////////////////////////////
/// @defgroup fw_params Framework Parameters
/// @brief These parameter defaults should be fine for most apps
////////////////////////////////////////////////////////////////////
///@{

int appVersion = 1;
int maxWifiFailCount = 5;
int wifiFailCountTimeLimit = 10;
int heartbeatInterval = 1800; // in seconds

///@}

// Should be /internal/iot/firmware
const char *firmwareUrl = "/firmware/";
const char *appRootUrl = "/internal/iot/";

char appSubTopic[100];

char minute[3] = "00";
char currentTime[6] = "00:00";
char meridian[3] = "AM";

int baseFontSize = 72;
int appNameFontSize = 56;
int friendlyNameFontSize = 24;
int appInstanceIDFontSize = 18;
int timeFontSize = 128;


int otherAppTopicCount = 0;
char otherAppTopic[10][25];
void (*otherAppMessageHandler[10])(char *topic, JsonDocument &doc);

////////////////////////////////////////////////////////////////////
/// @defgroup app_core_fns App Core Functions
/// @brief The framework depends on these functions. They may be
///        empty but may NOT be removed.
////////////////////////////////////////////////////////////////////
///@{
void printTimestamp(Print *_logOutput, int x);
void storePrefs();
void loadPrefs();
void ProcessMqttDisconnectTasks();
void ProcessMqttConnectTasks();
void ProcessWifiDisconnectTasks();
void ProcessWifiConnectTasks();
void appMessageHandler(char *topic, JsonDocument &doc);
void app_loop();
void app_setup();

///@}



// Example functions
void setupDisplay();
void initAppStrings();
void initWebServer();
bool checkGoodTime();
bool getNewTime();
void drawSplashScreen();
void drawTime();


//////////////////////////////////////////
//// Customizable Functions
//////////////////////////////////////////
void drawSplashScreen()
{
    String oldMethodName = methodName;
    methodName = "drawSplashScreen()";
    Log.verboseln("Entering");

    char showText[100];
    if (appInstanceID < 0)
    {
        sprintf(showText, "Configuring...");
    }
    else
    {
        sprintf(showText, "Name: %s", friendlyName);
    }
    sprintf(showText, "Device ID: %i", appInstanceID);
#ifdef USE_OPEN_FONT_RENDERER
    drawString(appName, screenCenterX, screenCenterY, appNameFontSize);

    drawString(showText, screenCenterX, screenCenterY + appNameFontSize / 2 + friendlyNameFontSize, friendlyNameFontSize);

    drawString(showText, screenCenterX, tft.height() - appInstanceIDFontSize / 2, appInstanceIDFontSize);
#endif

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

void setupFonts()
{
#ifdef USE_OPEN_FONT_RENDERER
    String oldMethodName = methodName;
    methodName = "setupDisplay()";
    Log.verboseln("Entering");

    ofr.setDrawer(tft);

    if (fontFS.exists("/fonts/Roboto-Regular.ttf"))
    {
        Log.infoln("Loading font from file.");
        if (ofr.loadFont("/fonts/Roboto-Regular.ttf"))
        {
            Log.errorln("Failed to load font from SPIFFS, loading from PROGMEM!!!");
            ofr.loadFont(Roboto, sizeof(Roboto));
        }
        else
        {
            Log.infoln("Loaded font from SPIFFS.");
        }
    }
    else
    {
        Log.errorln("Font does not exist on SPIFFS, loading from PROGMEM!!!");
        ofr.loadFont(Roboto, sizeof(Roboto));
    }

    ofr.setFontColor(TFT_WHITE, TFT_BLACK);
    ofr.setFontSize(baseFontSize);
    ofr.setAlignment(Align::MiddleCenter);

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
#endif
}

void setupDisplay()
{
    String oldMethodName = methodName;
    methodName = "setupDisplay()";
    Log.verboseln("Entering");

    Log.infoln("Setting up display.");

#ifdef USE_GRAPHICS
    tft.init();
    tft.setRotation(2);
    tft.fillScreen(TFT_BLACK);
#endif





    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

void initAppStrings()
{
    sprintf(appSubTopic, "%s/#", appName);
}




void ProcessWifiConnectTasks()
{
    String oldMethodName = methodName;
    methodName = "ProcessAppWifiConnectTasks()";
    Log.verboseln("Entering...");


#ifdef USE_WEB_SERVER
    initWebServer();
#endif

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

void ProcessWifiDisconnectTasks()
{
    String oldMethodName = methodName;
    methodName = "ProcessAppWifiDisconnectTasks()";
    Log.verboseln("Entering...");

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

void ProcessMqttConnectTasks()
{
    String oldMethodName = methodName;
    methodName = "ProcessMqttConnectTasks()";
    Log.verboseln("Entering...");

    uint16_t packetIdSub1 = mqttClient.subscribe(appSubTopic, 2);
    if (packetIdSub1 > 0)
        Log.infoln("Subscribing to %s at QoS 2, packetId: %u", appSubTopic, packetIdSub1);
    else
        Log.errorln("Failed to subscribe to %s!!!", appSubTopic);

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

void ProcessMqttDisconnectTasks()
{
    String oldMethodName = methodName;
    methodName = "ProcessMqttDisconnectTasks()";
    Log.verboseln("Entering...");

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

void appMessageHandler(char *topic, JsonDocument &doc)
{
    String oldMethodName = methodName;
    methodName = "appMessageHandler()";
    Log.verboseln("Entering...");

    // Add your implementation here
    char topics[10][25];
    int topicCounter = 0;
    char *token = strtok(topic, "/");

    while ((token != NULL) && (topicCounter < 11))
    {
        strcpy(topics[topicCounter++], token);
        token = strtok(NULL, "/");
    }

    // We can assume the first 2 subtopics are the appName and the appInstanceID
    // The rest of the subtopics are the command

    if (topics[2] == "reset")
    {
        reboot("Reset command received from MQTT");
    }
    else if (topics[2] == "set")
    {
        // TODO: Do something here
        if (topicCounter > 3)
        {
            // Only one parameter can be set this way
            // It should be topics[4]
            // But we should check that there are no more topics after that
            // If there are we should reject the command as malformed
        }
        else
        {
        // Need to scan the JSON to see what parameters the user
        // wants to set
        }
    }
    else if (topics[2] == "get")
    {
        // TODO: Do something here
    }

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
    return;
}

void loadPrefs()
{
    String oldMethodName = methodName;
    methodName = "loadPrefs()";
    Log.verboseln("Entering...");

    bool doesExist = preferences.isKey("appInstanceID");
    if (doesExist)
    {
        Log.infoln("Loading settings.");
        appInstanceID = preferences.getInt("appInstanceID");
        volume = preferences.getInt("Volume");
        bootCount = preferences.getInt("BootCount");
        preferences.getString("FriendlyName", friendlyName, 100); // 100 is the max length of the string
        // enableSnapshot = preferences.getBool("EnableSnapshot");
    }
    else
    {
        Log.warningln("Could not find Preferences!");
        Log.noticeln("appInstanceID not set yet!");
    }

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

void storePrefs()
{
    String oldMethodName = methodName;
    methodName = "storePrefs()";
    Log.verboseln("Entering...");

    Log.infoln("Storing Preferences.");

    preferences.putInt("appInstanceID", appInstanceID);
    preferences.putInt("Volume", volume);
    preferences.putInt("BootCount", bootCount);
    preferences.putString("FriendlyName", friendlyName);
    // preferences.putBool("EnableSnapshot", enableSnapshot);

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}


void printTimestamp(Print *_logOutput, int x)
{
    char c[20];
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    if (timeinfo->tm_year == 70)
    {
        sprintf(c, "%10lu ", millis());
    }
    else
    {
        strftime(c, 20, "%Y%m%d %H:%M:%S", timeinfo);
    }
    _logOutput->print(c);
    _logOutput->print(": ");
    _logOutput->print(methodName);
    _logOutput->print(": ");
}

bool checkGoodTime()
{
    String oldMethodName = methodName;
    methodName = "checkGoodTime()";
    Log.verboseln("Entering...");

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Log.errorln("Failed to obtain time");
        return false;
    }

    if (timeinfo.tm_year < (2020 - 1900))
    {
        Log.errorln("Failed to obtain time");
        return false;
    }

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
    return true;
}

bool getNewTime()
{
    String oldMethodName = methodName;
    methodName = "getNewTime()";
    Log.verboseln("Entering...");

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Log.errorln("Failed to obtain time");
        Log.verboseln("Exiting...");
        methodName = oldMethodName;
        return false;
    }

    char newTime[6] = "00:00";
    strftime(newTime, 20, "%I:%M", &timeinfo);
    strftime(meridian, 3, "%p", &timeinfo);

    if (strcmp(newTime, currentTime) != 0)
    {
        strcpy(currentTime, newTime);
        Log.infoln("Time is now %s %s", currentTime, meridian);
        Log.verboseln("Exiting...");
        methodName = oldMethodName;
        return true;
    }

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
    return false;
}

void redrawScreen()
{
    drawTime();
}

void drawTime()
{
    String oldMethodName = methodName;
    methodName = "drawTime()";
    Log.verboseln("Entering...");

#ifdef USE_GRAPHICS
    tft.fillScreen(TFT_BLACK);

#ifdef USE_OPEN_FONT_RENDERER
    drawString(currentTime, screenCenterX, screenCenterY, timeFontSize);
#endif

#endif


    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

IRAM_ATTR void interruptService()
{
}

void app_setup()
{
    String oldMethodName = methodName;
    methodName = "app_setup()";
    Log.verboseln("Entering...");

    // Add some custom code here
    initAppStrings();

    // Configure Hardware
    Log.infoln("Configuring hardware.");
    // pinMode(DOORBELL_PIN, INPUT);
    // attachInterrupt(digitalPinToInterrupt(DOORBELL_PIN), doorbellPressed, FALLING);

    File root = SD.open("/");
    if (!root)
    {
        Log.errorln("Failed to open directory");
    }
    else
    {
        printDirectory(root, 0);
    }



    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

////////////////////////////////////////////////////////////////////
/// @fn void app_loop()
/// @brief Called by the framework once per loop after all framework
/// processing is complete
////////////////////////////////////////////////////////////////////

void app_loop()
{

    if ((millis() % 1000) == 0)
    {
        if (!isGoodTime)
        {
            if (!(isGoodTime = checkGoodTime()))
                Log.infoln("Time not set yet.");
        }

#ifdef USE_GRAPHICS

        if (isFirstDraw)
        {
            isFirstDraw = false;
            clearScreen();
            redrawScreen();
        }

        // put your main code here, to run repeatedly:
        if (getNewTime())
        {
            drawTime();
        }
#endif
    }
}
#endif // APP_FUNCTIONS_H