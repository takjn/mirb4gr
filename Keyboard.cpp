/* USB Host support */
#include <hidboot.h>

uint8_t last_key = 0;
uint8_t get_last_key() { uint8_t ret = last_key; last_key = 0; return ret; }

class KbdRptParser : public KeyboardReportParser {
  protected:
    void OnKeyDown(uint8_t mod, uint8_t key);
};
void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key)
{
  // Serial.print(key);
  // WIP: temporarily code
  if (key == 40) {
    last_key = 13;  // Enter
  }
  else if (key == 42 || key == 76) {
    last_key = 127;  // Backspace or Delete
  }
  else {
    last_key = OemToAscii(mod, key);
  }
}

USB Usb;
HIDBoot<HID_PROTOCOL_KEYBOARD>    HidKeyboard(&Usb);
KbdRptParser KbdPrs;

void
setup_keyboard(void)
{
  if (Usb.Init() == -1) {
    Serial.println("OSC did not start.");
  }

  delay( 200 );

  HidKeyboard.SetReportParser(0, (HIDReportParser*)&KbdPrs);
}

void keyboard_task(void)
{
  Usb.Task();
}