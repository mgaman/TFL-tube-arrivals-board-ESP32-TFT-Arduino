#include "CheckBoxWidget.h"

/***************************************************************************************
** Code for a crude CheckBox element
**
**  
***************************************************************************************/
CheckBoxWidget::CheckBoxWidget(TFT_eSPI *tft)
{
  _tft = tft;
  _label[9] = '\0';
  _checked = false;
}

/// @brief  Create a checkbox, initially set to unchecked, Label (8 chars max) to the right
/// @param x1 Top Left position
/// @param y1 Top Left position
/// @param w Width (Height set to the same)
/// @param outline Outline color
/// @param fill  Fill color
/// @param textcolor Label text color
/// @param textbg label text background color
/// @param label Label (8 characters maximum)
/// @param textsize TFT_eSPI number
void CheckBoxWidget::initCheckBox(int16_t x1, int16_t y1, uint16_t w, uint16_t outline, uint16_t fill, uint16_t textcolor, uint16_t textbg, const char *label, uint8_t textsize,bool chk)
{
  _x1 = x1;
  _y1 = y1;
  _w = w;
  _outlinecolor = outline;
  _outlinewidth = 2;
  _fillcolor = fill;
  _textcolor = textcolor;
  _bgcolor = textbg;
  _textsize = textsize;
  strncpy(_label, label, 9);
  _checked = chk;
}

/// @brief Draw the box defined in initCheckBox
/// @param long_name Define a longer label if the 8 character limit in initCheckBox is too small
void CheckBoxWidget::drawCheckBox(String long_name)
{
  _tft->fillRect(_x1, _y1, _w, _w, _fillcolor);
  _tft->drawRect(_x1, _y1, _w, _w, _outlinecolor);
  _tft->setTextColor(_textcolor,_bgcolor);
  _tft->setTextSize(_textsize);
    int fh = _tft->fontHeight();
    if (long_name == "")
    {
      _tft->drawString(_label, _x1 + _w + 2, _y1+(_w/2)-(fh/2));
    }
    else
    {
      _tft->drawString(long_name, _x1 + _w + 2, _y1+(_w/2)-(fh/2));
    }
  if (_checked) {
  // if true draw X inside box else clear box (opposite of press())
    _tft->drawLine(_x1,_y1, _x1+_w,_y1+_w,_textcolor);
    _tft->drawLine(_x1,_y1+_w,_x1+_w,_y1,_textcolor);
  }
  else {
    _tft->fillRect(_x1, _y1, _w, _w, _fillcolor);  
    _tft->drawRect(_x1,_y1,_w,_w,_outlinecolor);
  }
}

/// @brief  Check of the given point is within the bounds of the CheckBox
/// @param x Point co-ordinate
/// @param y Point co-ordinate
/// @return True if yes, else false
bool CheckBoxWidget::contains(int16_t x, int16_t y)
{
  return ((x >= _x1) && (x < (_x1 + _w)) &&
          (y >= _y1) && (y < (_y1 + _w)));
}

void CheckBoxWidget::press() {
  // if true draw X inside box else clear box
  if (!_checked) {
    _tft->drawLine(_x1,_y1, _x1+_w,_y1+_w,_textcolor);
    _tft->drawLine(_x1,_y1+_w,_x1+_w,_y1,_textcolor);
  }
  else {
    _tft->fillRect(_x1, _y1, _w, _w, _fillcolor);  
    _tft->drawRect(_x1,_y1,_w,_w,_outlinecolor);
  }
  _checked = !_checked;
}

/// @brief Check state of checkbox
/// @return True if checked, else false
bool CheckBoxWidget::checked() {
  return _checked;
}
