/***************************************************************************************
// The following checkbox class was inspired by Bodmer widgets.
// 
//  Instance defines position & size on screen plus color of label and box
//  Label is always displayed to the right of the checkbox
//
//  Upon clicking the box its contents toggle between blank (unchecked) and a cross X (checked)
//
// The library uses functions present in TFT_eSPI
// https://github.com/Bodmer/TFT_eSPI
***************************************************************************************/
#ifndef _CheckBoxWidgetH_
#define _CheckBoxWidgetH_

//Standard support
#include <Arduino.h>

#include <TFT_eSPI.h>

class CheckBoxWidget : public TFT_eSPI {

 public:

  CheckBoxWidget(TFT_eSPI *tft);
  void     initCheckBox(int16_t x, int16_t y, uint16_t w, uint16_t outline, uint16_t fill, uint16_t textcolor, uint16_t textbg, const char *label, uint8_t textsize,bool chk = false);
  void     drawCheckBox( String long_name = "");
  bool     contains(int16_t x, int16_t y);  
  void     press();
  bool    checked();

 private:
  TFT_eSPI *_tft;
  int16_t  _x1, _y1;              // Coordinates of top-left corner of button
//  int16_t  _xd, _yd;              // Button text datum offsets (wrt centre of button)
  uint16_t _w;                // Width and height of button
  uint8_t  _textsize;
  //uint8_t  _textdatum; // Text size multiplier and text datum for button
  uint16_t _outlinecolor, _fillcolor, _textcolor, _outlinewidth, _bgcolor;
  char     _label[10]; // Button text is 9 chars maximum unless long_name used
  bool _checked;
};

#endif
