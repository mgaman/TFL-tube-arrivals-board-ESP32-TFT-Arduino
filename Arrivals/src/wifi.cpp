#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <List.hpp>
#include <ArduinoJson.h>
#include "AP.h"
#include "display.h"

List<AP> aplist;
//extern StaticJsonDocument<CONFIG_SIZE> config;
extern DynamicJsonDocument config;
WiFiUDP ntpUDP;
hw_timer_t *My_timer = NULL;

bool timerTicked = false;
time_t epoch;

void IRAM_ATTR onTimer()
{
    timerTicked = true;
    epoch++;
}

/// @brief
/// @param l
/// @param r
/// @return 0 if equal 1 if greater -1 if less
int compareRSSI(const void *l, const void *r)
{
    int lv = ((AP *)l)->getRSSI();
    int rv = ((AP *)r)->getRSSI();
    if (lv == rv)
        return 0;
    else
        return lv < rv ? 1 : -1;
}

/**
 * @brief  Scan all access points and make a list sorted by RSSI
 *
 */
bool wifiInit()
{
    // scan local wifi providers
    bool rc = false;
    int numSsid = WiFi.scanNetworks();
    if (numSsid != -1)
    {
#ifdef DEBUG
        Serial.print(numSsid);
        Serial.println(" networks");
#endif
        for (int i = 0; i < numSsid; i++)
        {
#ifdef DEBUG
            Serial.print(WiFi.SSID(i));
            Serial.print(": ");
            Serial.println(WiFi.RSSI(i));
#endif
            aplist.add(AP(WiFi.SSID(i).c_str(), WiFi.RSSI(i)));
        }
        // now sort the list, the lower the RSSI the stronger the signal
        aplist.sort(compareRSSI);
        rc = true;
    }
    return rc;
}

/**
 * @brief Using list created in wifiInit look for matching kei in config["iFi"]
 *
 */
bool wifiConnect()
{
    bool rc = false;
    for (int i = 0; i < aplist.getSize(); i++)
    {
        const char *ssid = aplist[i].getSSID();
        Serial.printf("%s %d \r\n", ssid, aplist[i].getRSSI());
        // check if this ssid in config, if so use it to connect to wifi
        if (config["WiFi"].containsKey(ssid))
        {
            Serial.printf("Connect with SSID %s\r\n", ssid);
            // get wifi connection
            WiFi.disconnect();
            WiFi.mode(WIFI_STA); // Optional
            WiFi.begin(ssid, (const char *)config["WiFi"][ssid]);
            Serial.println("\nConnecting");

            while (WiFi.status() != WL_CONNECTED)
            {
                Serial.print(".");
                delay(100);
            }
            Serial.println("\nConnected to the WiFi network");
            Serial.print("Local ESP32 IP: ");
            Serial.println(WiFi.localIP());
            displayClear();
            addLine(0, 1, WiFi.localIP().toString().c_str(), true);
            rc = true;
        }
        else
            Serial.printf("No key %s\r\n", ssid);
    }
    return rc;
}

void clockInit()
{
    // set up a 1PPS timer interrupt
    My_timer = timerBegin(0, 80, true);
    timerAttachInterrupt(My_timer, &onTimer, true);
    timerAlarmWrite(My_timer, 1000000, true);
    timerAlarmEnable(My_timer);
    bool dst = config["DST"];
    NTPClient timeClient = NTPClient(ntpUDP,(const char *)config["NTP_server"],dst ? 3600 :0);
    timeClient.begin();
    bool gotTime = false;
    while (!gotTime)
        gotTime = timeClient.update();
    epoch = timeClient.getEpochTime();
}

char orario[80];
void clockUpdate() {
  struct tm timeNow;
  if (timerTicked) {
    timerTicked = false;
    //epoch++;
    memcpy(&timeNow, localtime(&epoch), sizeof (struct tm));
//    strftime(orario,80,"%d/%m/%y -- %H:%M:%S", &timeNow);
    strftime(orario,80,"%H:%M:%S", &timeNow);
//    Serial.println(orario);
    displayBottomLine(orario,true);
  }
}