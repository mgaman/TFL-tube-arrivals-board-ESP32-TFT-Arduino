#include <Arduino.h>
#include <List.hpp>
#include <ArduinoJson.h>
#include "Item.h"
#include <TFT_eSPI.h>
#include <TFT_eWidget.h> // Widget library
#ifdef TFT_TOUCH
#include <TFT_Touch.h>
#endif
#include "Adafruit_GFX.h"
#include <CheckBoxWidget.h>
#include "display.h"
#include "jsonHandler.h"
#include "London_Underground_Regular10pt7b.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite bottomLine = TFT_eSprite(&tft);
// TFT_eSprite trainData = TFT_eSprite(&tft);
#ifdef TFT_TOUCH
TFT_Touch touch = TFT_Touch(16, 16, 16, 16); // just a placeholder, put unused GPIO
#endif

extern DynamicJsonDocument config;
extern bool configUpdated;

uint16_t SCREEN_BACKGROUND_COLOR;
uint16_t TEXT_FG_COLOR;
uint16_t TEXT_BG_COLOR;

extern int defaultStation;
static char buff[30]; // for local sprintf  3 chars platform name 22 chars destination

#define LINE_HEIGHT (tft.height() / 8) // total 8 lines on screen
#define INCREMENT true                 // for sorting algorithm

void displayInit()
{
  tft.begin();
  tft.setRotation(1); // landscape
  // extract colors from config
  // cannot express hex numbers in json so convert from hex string
  sscanf(config["StandardColors"][(const char *)config["SCREEN_COLOR"]], "%hx", &SCREEN_BACKGROUND_COLOR);
  sscanf(config["StandardColors"][(const char *)config["TEXT_FG_COLOR"]], "%hx", &TEXT_FG_COLOR);
  sscanf(config["StandardColors"][(const char *)config["TEXT_BG_COLOR"]], "%hx", &TEXT_BG_COLOR);
  tft.setTextColor(TEXT_FG_COLOR, TEXT_BG_COLOR);
  // font 4 for startup screen, change to fancy font later
  tft.setTextFont(config["FONT_NUMBER"]);
  //  tft.setFreeFont(&London_Underground_Regular10pt7b);
  tft.fillScreen(SCREEN_BACKGROUND_COLOR);
  bottomLine.createSprite(tft.width(), LINE_HEIGHT);
  bottomLine.setTextColor(TEXT_FG_COLOR, TEXT_BG_COLOR);
  bottomLine.fillSprite(SCREEN_BACKGROUND_COLOR);
  displayClear();

#ifdef TFT_TOUCH
  byte DCS = config["TouchScreenPins"]["DCS"];
  byte DCLK = config["TouchScreenPins"]["DCLK"];
  byte DIN = config["TouchScreenPins"]["DIN"];
  byte DOUT = config["TouchScreenPins"]["DOUT"];
  touch = TFT_Touch(DCS, DCLK, DIN, DOUT);
  touch.setCal(config["touchCalibration"][0], config["touchCalibration"][1], config["touchCalibration"][2], config["touchCalibration"][3],
               config["touchCalibration"][4], config["touchCalibration"][5], config["touchCalibration"][6]); // calibrated my screen
#endif
#ifdef TOUCH_SPI
  uint16_t calData[5];
  for (int k = 0; k < 5; k++)
    calData[k] = config["touchCalibration"][k];
  tft.setTouch(calData);
#endif
}

void displayInitFinish()
{
  // move to nicer font
  tft.setFreeFont(&London_Underground_Regular10pt7b);
}
/// @brief
/// @param l
/// @param r
/// @return 0 if equal 1 if greater -1 if less
int compare(const void *l, const void *r)
{
  int lv = ((Item *)l)->getArrivalTime();
  int rv = ((Item *)r)->getArrivalTime();
  if (lv == rv)
    return 0;
  else if (INCREMENT)
    return lv > rv ? 1 : -1;
  else
    return lv < rv ? 1 : -1;
}

int rowNumber = 0;
void displayClear()
{
  tft.fillScreen(SCREEN_BACKGROUND_COLOR);
  tft.setTextColor(TEXT_FG_COLOR, TEXT_BG_COLOR);
  rowNumber = 1; // skip over title line
}

/// @brief Check if one of the paramaters has a shortened version
/// @param destinationName Note, may be zero length
/// @param towards   Note may be zero length
/// @return Shortname
const char *getShortName(const char *destinationName, const char *towards)
{
  const char *shortName = NULL;
  // use towards if present
  const char *longName = strlen(towards) > 0 ? towards : destinationName;
#ifdef DEBUG
  Serial.printf("D %s T %s L %s\r\n", destinationName, towards, longName);
#endif
  if (config["substitutes"].containsKey(longName))
  {
    shortName = config["substitutes"][longName];
#ifdef DEBUG
    Serial.printf("long %s short %s\r\n", longName, shortName);
#endif
  }
  else
  {
    shortName = longName;
#ifdef DEBUG
    Serial.printf("No sub %s \r\n", longName);
#endif
  }
  return shortName;
}

void displayItem(List<Item> *pLI)
{
  // check if any arrivals on desired platform(s)
  if (pLI->getSize() > 0)
  {
    pLI->sort(compare);
    for (int j = 0; j < config["stations"][defaultStation]["rowsPerPlatform"]; j++)
    {
      Item *pI = pLI->getPointer(j);
      // Arrival time round down seconds to minutes
      int aTime = pI->getArrivalTime() / 60;
      const char *dest = getShortName(pI->getDestination(), pI->getTowards());
      if (dest == NULL)
        dest = (char *)pI->getDestination();

      sprintf(buff, "%d %s", pI->getPlatform(), dest);
      addLine(0, rowNumber, buff, false);
      sprintf(buff, "%2d", aTime);
      addLine(tft.width() - tft.textWidth(buff) - 3, rowNumber, buff, false);
#ifdef DEBUG
      //      Serial.printf("platform %d ETA %d dest %s\r\n",pI->getPlatform(),pI->getArrivalTime(),pI->getDestination());
      Serial.print("platform ");
      Serial.print(pI->getPlatform());
      Serial.print(" ETA ");
      Serial.print(pI->getArrivalTime());
      Serial.print(" dest ");
      Serial.println(pI->getDestination());
#endif
      rowNumber++;
    }
  }
}

/// @brief Add a line to the screen
/// @param col Column in pixels
/// @param row Row number (in rows, not pixels)
/// @param text text
/// @param centered  true if text to be centered (col ignored), else false
void addLine(int col, int row, const char *text, bool centered = false)
{
  int textw;
  if (centered)
  {
    // cannot use default font with drawCentreString so do some maths
    int l = tft.textWidth(text);
    textw = tft.drawString(text, (tft.width() - l) / 2, row * LINE_HEIGHT);
  }
  else
    textw = tft.drawString(text, col, row * LINE_HEIGHT);
}

#define BUTTON_W 100 // needed in several functions
#define BUTTON_H 50
#define CB_SIDE 40

ButtonWidget btnL = ButtonWidget(&tft); // need to be global
ButtonWidget btnR = ButtonWidget(&tft);
ButtonWidget btnC = ButtonWidget(&tft);
CheckBoxWidget defaultCB = CheckBoxWidget(&tft);
CheckBoxWidget defaultTZ = CheckBoxWidget(&tft);

// Create an array of button instances to use in for() loops
// This is more useful where large numbers of buttons are employed
ButtonWidget *btn[] = {&btnL, &btnR, &btnC};
uint8_t buttonCount = sizeof(btn) / sizeof(btn[0]);

TFT_eSprite banner = TFT_eSprite(&tft); // Sprite for banner heading
static int stationIndex = 0;
int STATION_LIMIT;
bool stationChosen = false;

void stationUpdate(int x)
{
  stationIndex = stationIndex + x;
  // check for wrap around
  if (stationIndex < 0)
    stationIndex = STATION_LIMIT - 1;
  else if (stationIndex == STATION_LIMIT)
    stationIndex = 0;
#ifdef DEBUG
  Serial.println(stationIndex);
#endif

  // Push Sprite image to the TFT screen at x, y
  banner.fillSprite(SCREEN_BACKGROUND_COLOR);
  banner.drawCentreString((const char *)config["stations"][stationIndex]["Name"], tft.width() / 2, 0, (int)config["FONT_NUMBER"]);
  banner.pushSprite(0, 0); // Plot sprite
}

void btnL_pressAction(void)
{
  if (btnL.justPressed())
  {
#ifdef DEBUG
    Serial.println("Left button just pressed");
#endif
    btnL.drawSmoothButton(true);
  }
}

void btnL_releaseAction(void)
{
  static uint32_t waitTime = 1000;
  if (btnL.justReleased())
  {
#ifdef DEBUG
    Serial.println("Left button just released");
#endif
    btnL.drawSmoothButton(false);
    btnL.setReleaseTime(millis());
    waitTime = 10000;
    stationUpdate(-1);
  }
  else
  {
    if (millis() - btnL.getReleaseTime() >= waitTime)
    {
      waitTime = 1000;
      btnL.setReleaseTime(millis());
      btnL.drawSmoothButton(!btnL.getState());
    }
  }
}

void btnR_pressAction(void)
{
  if (btnR.justPressed())
  {
    btnR.drawSmoothButton(!btnR.getState(), 3, SCREEN_BACKGROUND_COLOR, btnR.getState() ? "OFF" : "ON");
#ifdef DEBUG
    Serial.print("Button toggled: ");
#endif
    if (btnR.getState())
      Serial.println("ON");
    else
      Serial.println("OFF");
    btnR.setPressTime(millis());
  }

  // if button pressed for more than 1 sec...
  if (millis() - btnR.getPressTime() >= 1000)
  {
    Serial.println("Stop pressing my buttton.......");
  }
  else
    Serial.println("Right button is being pressed");
}

void btnR_releaseAction(void)
{
  static uint32_t waitTime = 1000;
  if (btnR.justReleased())
  {
#ifdef DEBUG
    Serial.println("Right button just released");
#endif
    btnR.drawSmoothButton(false);
    btnR.setReleaseTime(millis());
    waitTime = 10000;
    stationUpdate(1);
  }
  else
  {
    if (millis() - btnR.getReleaseTime() >= waitTime)
    {
      waitTime = 1000;
      btnR.setReleaseTime(millis());
      btnR.drawSmoothButton(!btnR.getState());
    }
  }
}

void btnC_pressAction(void)
{
  if (btnC.justPressed())
  {
    btnC.drawSmoothButton(!btnC.getState(), 3, SCREEN_BACKGROUND_COLOR, btnC.getState() ? "OFF" : "ON");
#ifdef DEBUG
    Serial.print("Button C toggled: ");
#endif
    if (btnC.getState())
      Serial.println("ON");
    else
      Serial.println("OFF");
    btnC.setPressTime(millis());
  }

  // if button pressed for more than 1 sec...
  if (millis() - btnC.getPressTime() >= 1000)
  {
    Serial.println("Stop pressing my buttton.......");
  }
  else
    Serial.println("Center button is being pressed");
}

void btnC_releaseAction(void)
{
  static uint32_t waitTime = 1000;
  if (btnC.justReleased())
  {
#ifdef DEBUG
    Serial.println("Center button just released");
#endif
    btnC.drawSmoothButton(false);
    btnC.setReleaseTime(millis());
    waitTime = 10000;
    stationChosen = true;
  }
  else
  {
    if (millis() - btnC.getReleaseTime() >= waitTime)
    {
      waitTime = 1000;
      btnC.setReleaseTime(millis());
      btnC.drawSmoothButton(!btnC.getState());
    }
  }
}
/// @brief  Init buttons and checkbox
void initButtons()
{
  // 3 buttons in a row along the bottom
  uint16_t y = tft.height() - BUTTON_H - 10; // y is constant
  uint16_t x = 10;
  btnL.initButtonUL(x, y, BUTTON_W, BUTTON_H, TFT_WHITE, SCREEN_BACKGROUND_COLOR, TFT_GREEN, "<--", 1);
  btnL.setPressAction(btnL_pressAction);
  btnL.setReleaseAction(btnL_releaseAction);
  btnL.drawSmoothButton(false, 3, SCREEN_BACKGROUND_COLOR);

  x = tft.width() - BUTTON_W - 10;
  btnR.initButtonUL(x, y, BUTTON_W, BUTTON_H, TFT_WHITE, SCREEN_BACKGROUND_COLOR, TFT_GREEN, "-->", 1);
  btnR.setPressAction(btnR_pressAction);
  btnR.setReleaseAction(btnR_releaseAction);
  btnR.drawSmoothButton(false, 3, SCREEN_BACKGROUND_COLOR);

  x = tft.width() / 2 - (BUTTON_W / 2);
  btnC.initButtonUL(x, y, BUTTON_W, BUTTON_H, TFT_WHITE, TEXT_FG_COLOR, SCREEN_BACKGROUND_COLOR, "Select", 1);
  btnC.setPressAction(btnC_pressAction);
  btnC.setReleaseAction(btnC_releaseAction);
  btnC.drawSmoothButton(false, 3, SCREEN_BACKGROUND_COLOR);

  // place checkbox above buttons and to the left
  x = 10;
  y = tft.height() - BUTTON_H - 10 - (CB_SIDE * 2);
  defaultCB.initCheckBox(x, y, CB_SIDE, TFT_WHITE, TFT_BLACK, TFT_YELLOW, TFT_BLACK, "set sta", 1,config["defaultStation"] >= 0);
  defaultCB.drawCheckBox("set station as default");

  defaultTZ.initCheckBox(x, y - CB_SIDE - 2, CB_SIDE, TFT_WHITE, TFT_BLACK, TFT_YELLOW, TFT_BLACK, "set TZ", 1,(bool)config["DST"]);
  defaultTZ.drawCheckBox("Daylight Savings Time");
}

int touchHandler()
{
  /* Create an instance of the touch screen library */
// 1bpp Sprites are economical on memory but slower to render
#define COLOR_DEPTH 1 // Colour depth (1, 8 or 16 bits per pixel)
  banner.setColorDepth(COLOR_DEPTH);
  banner.createSprite(tft.width(), 20);
  banner.fillSprite(SCREEN_BACKGROUND_COLOR);

  // Set the 2 pixel palette colours that 1 and 0 represent on the display screen
  // banner.setBitmapColor(TFT_WHITE, TFT_BLACK);

  // Calibrate the touch screen and retrieve the scaling factors
  initButtons();
  STATION_LIMIT = config["stations"].size(); // getNumberOfStations();
  // start with current default station, if any, else 0
  stationUpdate(config["defaultStation"] < 0 ? 0 : config["defaultStation"]);

  uint32_t scanTime = millis();
  // loop until center button clicked
  while (!stationChosen)
  {
    uint16_t t_x = 9999, t_y = 9999; // To store the touch coordinates

    // Scan keys every 50ms at most
    if (millis() - scanTime >= 50)
    {
      // Pressed will be set true if there is a valid touch on the screen
#ifdef TFT_TOUCH
      bool pressed = touch.Pressed();
#endif
#ifdef TOUCH_SPI
      // Pressed will be set true is there is a valid touch on the screen
      bool pressed = tft.getTouch(&t_x, &t_y);
#endif
      scanTime = millis();
      for (uint8_t b = 0; b < buttonCount; b++)
      {
        if (pressed)
        {
#ifdef TFT_TOUCH
          t_x = touch.X();
          t_y = touch.Y();
#endif
          if (btn[b]->contains(t_x, t_y))
          {
            btn[b]->press(true);
            btn[b]->pressAction();
          }
        }
        else
        {
          btn[b]->press(false);
          btn[b]->releaseAction();
        }
      }
      if (pressed && defaultCB.contains(t_x, t_y))
      {
        defaultCB.press();
        configUpdated = true;
      }

      if (pressed && defaultTZ.contains(t_x, t_y))
      {
        defaultTZ.press();
        configUpdated = true;
      }
    }
  }
  if (defaultCB.checked())
    config["defaultStation"] = stationIndex;
  else
    config["defaultStation"] = -1;
  config["DST"] = (bool)defaultTZ.checked();
  return stationIndex;
}

void displayBottomLine(const char *text, bool centered)
{
  // text contains : that london transport font cannot print, so force font 4 here
  bottomLine.fillSprite(SCREEN_BACKGROUND_COLOR);
  if (centered)
    bottomLine.drawCentreString(text, tft.width() / 2, 0, config["FONT_NUMBER"]);
  else
    bottomLine.drawString(text, 0, 0, config["FONT_NUMBER"]);
  bottomLine.pushSprite(0, 7 * LINE_HEIGHT);
}

int selectDefaultStation()
{
  int selected = config["defaultStation"];
#ifdef TFT_TOUCH
  bool pressed = touch.Pressed();
#elif defined(TOUCH_SPI)
  uint16_t x, y;
  bool pressed = tft.getTouch(&x, &y);
#endif
  if (pressed || selected == -1)
  {
    selected = touchHandler();
    addLine(0, 1, "Selected", true);
  }
  return selected;
}
