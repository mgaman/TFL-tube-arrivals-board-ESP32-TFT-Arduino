#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#include "jsonHandler.h"
#include "display.h"

DynamicJsonDocument doc(DOC_SIZE);
DynamicJsonDocument config(CONFIG_SIZE);
StaticJsonDocument<FILTER_SIZE> filter;

extern int defaultStation;
extern HTTPClient client;

void processAll();

bool jsonHandler(/*String raw*/)
{
    bool rc = false;
    if (!doc.isNull())
        doc.clear();   // not nesessary according to documentation
    DeserializationError derr =
        deserializeJson(doc, client.getStream()/*raw.c_str()*/, DeserializationOption::Filter(filter));
    switch (derr.code()) {
        case DeserializationError::Ok:
            if (config["logRawData"])
            {
                serializeJsonPretty(doc, Serial);
                Serial.printf("doc %d bytes\n", doc.memoryUsage());
            }
            processAll();
            rc = true;
            break;
        case DeserializationError::EmptyInput:
        case DeserializationError::IncompleteInput:
        case DeserializationError::InvalidInput:
        case DeserializationError::TooDeep:
            Serial.print("doc ");
            Serial.println(derr.c_str());
            break;
        case DeserializationError::NoMemory:
            displayClear();
            addLine(0,1,"increase DOC_SIZE",true);
            break;
    }
    return rc;
}

bool jsonInit(fs::FS &fs, const char *path)
{
    /*
        We cannot create config from a file stream as it needs to be writeable (to update wifi creds) so
        we read all the file into a buffer and keep the buffer in memory
    */
    bool rc = false;
    File file = fs.open(path);
    if (!file || file.isDirectory())
    {
        Serial.println("− failed to open file for reading");
    }
    else {
        int bsize = 10 + file.size();
        uint8_t *buff = (uint8_t *)malloc(bsize);
        if (buff)
        {
            file.read(buff, bsize);
            DeserializationError derr = deserializeJson(config, buff);
            switch (derr.code())
            {
                case DeserializationError::Ok:
                    if (config["exportJson"])
                    {
                        serializeJsonPretty(config, Serial);
                        Serial.printf("\r\nconfig %d bytes\n", config.memoryUsage());
                    }
                    // build the filter json object
                    for (int i = 0; i < config["filterKeys"].size(); i++)
                    {
                        filter[0][(const char *)config["filterKeys"][i]] = true;
                    }
                    if (config["logRawData"])
                    {
                        serializeJsonPretty(filter, Serial);
                        Serial.printf("filter %d bytes\n", filter.memoryUsage());
                    }
                    rc = true;
                    break;
                case DeserializationError::EmptyInput:
                case DeserializationError::IncompleteInput:
                case DeserializationError::InvalidInput:
                case DeserializationError::NoMemory:
                case DeserializationError::TooDeep:
                    Serial.print("config ");
                    Serial.println(derr.c_str());
                    break;
            }
        }
        else
            Serial.println("config.json malloc failed");
        //   free(buff);  // DO NOT free memory, config data is here
    }
    return rc;
}

bool jsonWriteback() {
    File file = SPIFFS.open("/config.json","w");
    return serializeJson(config,file) >= 0;
}
