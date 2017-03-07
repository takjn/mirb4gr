#ifndef DISPLAY_H
#define DISPLAY_H

#define I2C_ADDRESS 0x3C

void setup_display(void);
void handle_backspace(void);
void display_write(char c);
void display_print(const char *);
void display_print(int);
void display_println(const char *);

/* use Serial instead of stdout */
#define stdout_putc(c)           { Serial.write(c); display_write(c); }
#define stdout_write(s, len)     { Serial.write(s, len); for(int i=0;i<len;i++) display_write(s[i]); }
#define stdout_print(s)          { Serial.print(s); display_print(s); }
#define stdout_println(s)        { Serial.println(s); display_println(s); }

#endif