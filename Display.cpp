/* SSD1306Ascii - Text only Arduino Library for SSD1306 OLED displays */
#include "Display.h"
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

SSD1306AsciiWire oled;


void setup_display(void)
{
  Wire.begin();
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  oled.setFont(TomThumbs3x6);
  oled.setScroll(true);
  oled.clear();
}

void display_write(char c)
{
  oled.write(c);
}

void display_print(const char *s)
{
  oled.print(s);
}

void display_print(int i)
{
  oled.print(i);
}

void display_println(const char *s)
{
  oled.println(s);
}

void handle_backspace(void)
{
  oled.setCol(oled.col() - (oled.fontWidth() + 1));
  oled.clearToEOL();
}
