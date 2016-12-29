/* GR-CITRUS Sketch Template E1.00 */

#include <Arduino.h>

void setup(){
	pinMode(PIN_LED0, OUTPUT);
	
}

void loop(){
	
	digitalWrite(PIN_LED0, HIGH);
    delay(500);
	digitalWrite(PIN_LED0, LOW);
    delay(500);
	
}
