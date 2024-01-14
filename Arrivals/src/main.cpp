#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <FS.h>
#include <SPIFFS.h>
#include "jsonHandler.h"
#include "wifi.h"
#include "display.h"


bool jsonInit(fs::FS &fs,const char *path);
extern DynamicJsonDocument config;
int defaultStation;   // global value

#define FORMAT_SPIFFS_IF_FAILED true
char URL[100];
uint32_t nextGetJson;

HTTPClient client;
bool getJSON()
{
    bool rc = false;
    client.begin(URL);
    // http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String payload = emptyString;
    int httpCode = HTTP_CODE_OK + 1;  // i.e. not OK
    while (httpCode != HTTP_CODE_OK && payload.isEmpty()) {
        httpCode = client.GET();
        if (httpCode == HTTP_CODE_OK) {
            payload = client.getString();
            rc = jsonHandler(payload);
            client.end();
        }
        else
        {
            Serial.printf("[HTTP] GET... failed, error: %s\n", client.errorToString(httpCode).c_str());
            displayClear();
            addLine(0,1,"Missing Data", true);
            delay(1000);  // try again
        }
    }
    return rc;
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println();
    if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }
    if (jsonInit(SPIFFS,"/config.json")) {
        displayInit();   // must be AFTER jsonInit
        defaultStation = config["defaultStation"];
        if (defaultStation < 0)
            defaultStation = selectDefaultStation();
        // create the URL
        sprintf(URL,(const char *)config["URL"]["formatC"],(const char *)config["server"], (const char *)config["stations"][defaultStation]["ID"]);
#ifdef DEBUG
        Serial.println(URL);
#endif
        if (wifiInit()) {
            if (!wifiConnect()) {
                Serial.println("Failed to connect to WiFi");
                displayClear();
                addLine(0,1,"Failed to connect to WiFi",false);
                while (true);
            }
        }
        else {
            Serial.println("No WiFi available");
            displayClear();
            addLine(0,1,"No WiFi available",false);
            while (true) {}
        }
        clockInit();  // must be after wifi enabled
        nextGetJson = 0;
        displayInitFinish();
    }
    else {
        Serial.println("jsonInit failed");
        nextGetJson = 0x7fffffff;
    }
}

void loop()
{
    if (millis() > nextGetJson) {
        nextGetJson = millis() + ((unsigned)config["refreshDelaySeconds"]*1000);
        getJSON();
    }
    clockUpdate();
}
