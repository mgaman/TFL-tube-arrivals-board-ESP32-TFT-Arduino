#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <FS.h>
#include <SPIFFS.h>
#include <esp_task_wdt.h>
#include "jsonHandler.h"
#include "WiFiAr.h"
#include "display.h"


bool jsonInit(fs::FS &fs,const char *path);
extern DynamicJsonDocument config;
int defaultStation;   // global value
extern unsigned long lastUpdate;

#define FORMAT_SPIFFS_IF_FAILED true
char URL[100];
uint32_t nextGetJson;
bool configUpdated = false;

HTTPClient client;
bool getJSON()
{
    bool rc = false;
    char message[30];
    int attempts = config["httpRetries"];
    client.useHTTP10(true);   // ESSENTIAL else getStream() wont work
    client.begin(URL);
    // http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = HTTP_CODE_OK + 1;  // i.e. not OK
    while (attempts && (httpCode != HTTP_CODE_OK)) {
        httpCode = client.GET();
        if (httpCode == HTTP_CODE_OK) {
            rc = jsonHandler();
        }
        else
        {
            Serial.printf("[HTTP] GET... failed, error: %s\n", client.errorToString(httpCode).c_str());
            displayClear();
            addLine(0,1,"Missing Data", true);
            sprintf(message,"Elapsed %lu",lastUpdate/1000);
            addLine(0,2,message,true);
            delay(1000);  // try again
            attempts--;
        }
    }
    client.end();
    return rc;
}

void setup()
{
    char message[30];
    Serial.begin(115200);
    delay(1000);
    Serial.println();
    if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }
    if (jsonInit(SPIFFS,"/config.json")) {
        displayInit();   // must be AFTER jsonInit
        if (wifiInit()) {
            if (!wifiConnect()) {
                Serial.println("Failed to connect to WiFi");
                displayClear();
                addLine(0,1,"Failed to connect to WiFi",true);
                addLine(0,2,"Connect to access point",true);
                addLine(0,3,(const char *)config["accessPointName"],true);
                if (wifiManage()) {
                    delay(2000); // time to read message
                    displayClear();
                }
                else
                    while(true);
            }
            else { 
                delay(2000);   // time to see message
                displayClear();
            }
        }
        else {
            Serial.println("No WiFi available");
            displayClear();
            addLine(0,1,"No WiFi available",false);
            while (true) {}
        }
        defaultStation = selectDefaultStation();
        // create the URL
        sprintf(URL,(const char *)config["URL"]["formatC"],(const char *)config["server"], (const char *)config["stations"][defaultStation]["ID"]);
#ifdef DEBUG
        Serial.println(URL);
#endif
        clockInit();  // must be after wifi enabled
        nextGetJson = 0;
        displayInitFinish();
        if (configUpdated) {
            if (jsonWriteback())
                Serial.println("json updated");
            else {
                Serial.println("json update faiuled");
                displayClear();
                addLine(0,1,"JSON update failed",false);
            }
        }
        // set up watchdog
        esp_task_wdt_init((int)config["refreshDelaySeconds"]*2, true); //enable panic so ESP32 restarts
        esp_task_wdt_add(NULL); //add current thread to WDT watch
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
        if (!getJSON()) {
            Serial.printf("getJSON failed elapsed %lu\r\n",millis()/1000);
            nextGetJson = 0;  // force retry now     
        }
        else
            esp_task_wdt_reset();
    }
    clockUpdate();
}
