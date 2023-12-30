#include <Arduino.h>
#include <List.hpp>
#include <Wire.h>
#include <ArduinoJson.h>
#include "Item.h"
#include "display.h"

extern DynamicJsonDocument doc;
extern DynamicJsonDocument config;
extern int defaultStation;

void displayItem(List<Item> *pLI);

/***
 * Use config to filter data by platform number
 * Not that platform name is an inconsistent format, may be "Platform 4" or "Eastbound Platform 4"
 * We need a reliable way to extract the number only
 */
void processAll()
{
  // make a list for each platform
  int numItems =  config["stations"][defaultStation]["PlatformsToDisplay"].size();
  List<Item> li[numItems];
#ifdef DEBUG
  Serial.print("List size:");
  Serial.println(numItems);
#endif
  // read line by line & populate the appropriate list
  for (int row = 0; row < doc.size(); row++)
  {
 //   int pn = getArrivalPlatformNumber(row);
    // make an assumption here that text is xxxxxxnnn where x is non-digit and n is digit
    const char *c = doc[row]["platformName"];
    while (!isdigit(*c))
        c++; // skip non-digits
    int pn  = atoi(c);

    // check for match of platform number & place in config PlatformsToDisplay
    for (int itemIndex = 0; itemIndex < numItems; itemIndex++)
    {
    //  if (pn == getPlatformToDisplay(itemIndex))
      if (pn == config["stations"][defaultStation]["PlatformsToDisplay"][itemIndex])
      {
        // Have noticed cases where destinationName is missing
//        if (rowContainsKey(row,"destinationName")) {
#ifdef DEBUG
          Serial.printf("Add index %d %d %s %d\r\n", itemIndex, getETA(row), getDestinationName(row), pn);
#endif
//          li[itemIndex].add(Item(getETA(row), getDestinationName(row), getTowardsName(row), pn));
          li[itemIndex].add(Item(doc[row]["timeToStation"], doc[row]["destinationName"], doc[row]["towards"], pn));
         // doc[row]["timeToStation"];
  //      }
    //    else {
#ifdef DEBUG
      //    Serial.println("Missing destination name");
#endif          
        //}
      }
      else {
#ifdef DEBUG
        Serial.printf("no pn match %d %d %d\r\n",itemIndex,pn,getPlatformToDisplay(itemIndex));   
#endif  
      }
    }
  }

  displayClear();
  // send unsorted list (per platform)
  for (int item = 0; item < numItems; item++)
    displayItem(&li[item]);
}
