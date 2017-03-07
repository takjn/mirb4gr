#ifndef KEYBOARD_H
#define KEYBOARD_H

// Define serial port
#define Serial Serial1

uint8_t get_last_key(void);
void setup_keyboard(void);
void keyboard_task(void);

#endif
