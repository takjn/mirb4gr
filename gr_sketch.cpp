/* GR-CITRUS Sketch Template E1.00 */

#include <Arduino.h>
#include <mruby.h>
#include <mruby/compile.h>

void setup(){
	Serial.begin(115200);
	pinMode(PIN_LED0, OUTPUT);
}

void loop(){

	digitalWrite(PIN_LED0, HIGH);
    delay(500);
	digitalWrite(PIN_LED0, LOW);
    delay(500);

	Serial.println("Step1");
	mrb_state *mrb = mrb_open();
	Serial.println("Step2");
  if (!mrb) { 	Serial.println("mrb_open error"); }
  mrb_load_string(mrb, "puts 'hello world'");
	Serial.println("Step3");
  mrb_close(mrb);

}
