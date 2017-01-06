/*
** mruby memory allocated size checker
*/

#include "Arduino.h"
#include <mruby.h>

mrb_state *mrb;
size_t total_allocated;

// custom allocator to check heap shortage.
void *myallocf(mrb_state *mrb, void *p, size_t size, void *ud){
	if (size == 0){
		free(p);
		return NULL;
	}

	void *ret = realloc(p, size);
	if (!ret){
		/*
			Reaches here means mruby failed to allocate memory.
			Sometimes it is not critical because mruby core will retry allocation
			after GC.
		*/

		Serial.print("memory allocation error. requested size:");
		Serial.println(size, DEC);

		Serial.flush();

		//Ensure serial output received by host before proceeding.
		delay(200);
		return NULL;
	}
	total_allocated += size;
	return ret;
}

void setup() {
  Serial.begin(115200);

test:
  for (int i=0;i<10;i++) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("");

  //Init mruby
	Serial.println("mrb_open()...");
	delay(100);
	mrb = mrb_open_allocf(myallocf, NULL);
	if (mrb){
		Serial.println("mrb_open()...done.");
		Serial.print("total allocated : ");
		Serial.println(total_allocated,DEC);
    mrb_close(mrb);
    total_allocated = 0;
	}else{
		Serial.println("mrb_open()...failed.");
		Serial.print("total allocated : ");
		Serial.println(total_allocated,DEC);
		return;
	}

  goto test;
}

void loop() {

}
