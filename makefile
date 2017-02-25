include ./mruby/build/RX630/lib/libmruby.flags.mak

SRCFILES = ./gr_sketch.cpp ./gr_common/core/HardwareSerial.cpp ./gr_common/core/main.cpp ./gr_common/core/MsTimer2.cpp ./gr_common/core/new.cpp ./gr_common/core/Print.cpp ./gr_common/core/Stream.cpp ./gr_common/core/Tone.cpp ./gr_common/core/usbdescriptors.c ./gr_common/core/usb_cdc.c ./gr_common/core/usb_core.c ./gr_common/core/usb_hal.c ./gr_common/core/utilities.cpp ./gr_common/core/WInterrupts.c ./gr_common/core/wiring.c ./gr_common/core/wiring_analog.c ./gr_common/core/wiring_digital.c ./gr_common/core/wiring_pulse.c ./gr_common/core/wiring_shift.c ./gr_common/core/WMath.cpp ./gr_common/core/WString.cpp ./gr_common/core/avr/avrlib.c ./gr_common/lib/DSP/DSP.cpp ./gr_common/lib/EEPROM/EEPROM.cpp ./gr_common/lib/EEPROM/utility/r_flash_api_rx600.c ./gr_common/lib/Firmata/Firmata.cpp ./gr_common/lib/LiquidCrystal/LiquidCrystal.cpp ./gr_common/lib/RTC/RTC.cpp ./gr_common/lib/RTC/utility/RX63_RTC.cpp ./gr_common/lib/SD/File.cpp ./gr_common/lib/SD/SD.cpp ./gr_common/lib/SD/utility/Sd2Card.cpp ./gr_common/lib/SD/utility/SdFile.cpp ./gr_common/lib/SD/utility/SdVolume.cpp ./gr_common/lib/Servo/Servo.cpp ./gr_common/lib/SoftwareSerial/SoftwareSerial.cpp ./gr_common/lib/SPI/SPI.cpp ./gr_common/lib/Stepper/Stepper.cpp ./gr_common/lib/Wire/Wire.cpp ./gr_common/lib/Wire/utility/I2cMaster.cpp ./gr_common/lib/Wire/utility/twi_rx.c ./gr_common/rx63n/exception_handler.cpp ./gr_common/rx63n/hardware_setup.cpp ./gr_common/rx63n/interrupt_handlers.c ./gr_common/rx63n/reboot.c ./gr_common/rx63n/reset_program.asm ./gr_common/rx63n/util.c ./gr_common/rx63n/vector_table.c \
./USB_Host/adk.cpp ./USB_Host/BTD.cpp ./USB_Host/BTHID.cpp ./USB_Host/cdcacm.cpp ./USB_Host/cdcftdi.cpp ./USB_Host/cdcprolific.cpp ./USB_Host/hid.cpp ./USB_Host/hidboot.cpp ./USB_Host/hidescriptorparser.cpp ./USB_Host/hiduniversal.cpp ./USB_Host/hwDmaIf.c ./USB_Host/masstorage.cpp ./USB_Host/message.cpp ./USB_Host/parsetools.cpp ./USB_Host/r_usbh_driver.c ./USB_Host/SPP.cpp ./USB_Host/Usb.cpp ./USB_Host/usbhBulk.c ./USB_Host/usbhControl.c ./USB_Host/usbhDriver.c ./USB_Host/usbhInterrupt.c ./USB_Host/usbhIsochronous.c ./USB_Host/usbhMain.c ./USB_Host/usbhPipe.c ./USB_Host/usbhub.cpp ./USB_Host/utilities/sysif.c \
./SSD1306Ascii/src/SSD1306Ascii.cpp
OBJFILES = ./gr_sketch.o ./gr_common/core/HardwareSerial.o ./gr_common/core/main.o ./gr_common/core/MsTimer2.o ./gr_common/core/new.o ./gr_common/core/Print.o ./gr_common/core/Stream.o ./gr_common/core/Tone.o ./gr_common/core/utilities.o ./gr_common/core/WMath.o ./gr_common/core/WString.o ./gr_common/lib/DSP/DSP.o ./gr_common/lib/EEPROM/EEPROM.o ./gr_common/lib/Firmata/Firmata.o ./gr_common/lib/LiquidCrystal/LiquidCrystal.o ./gr_common/lib/RTC/RTC.o ./gr_common/lib/RTC/utility/RX63_RTC.o ./gr_common/lib/SD/File.o ./gr_common/lib/SD/SD.o ./gr_common/lib/SD/utility/Sd2Card.o ./gr_common/lib/SD/utility/SdFile.o ./gr_common/lib/SD/utility/SdVolume.o ./gr_common/lib/Servo/Servo.o ./gr_common/lib/SoftwareSerial/SoftwareSerial.o ./gr_common/lib/SPI/SPI.o ./gr_common/lib/Stepper/Stepper.o ./gr_common/lib/Wire/Wire.o ./gr_common/lib/Wire/utility/I2cMaster.o ./gr_common/rx63n/exception_handler.o ./gr_common/rx63n/hardware_setup.o ./gr_common/core/usbdescriptors.o ./gr_common/core/usb_cdc.o ./gr_common/core/usb_core.o ./gr_common/core/usb_hal.o ./gr_common/core/WInterrupts.o ./gr_common/core/wiring.o ./gr_common/core/wiring_analog.o ./gr_common/core/wiring_digital.o ./gr_common/core/wiring_pulse.o ./gr_common/core/wiring_shift.o ./gr_common/core/avr/avrlib.o ./gr_common/lib/EEPROM/utility/r_flash_api_rx600.o ./gr_common/lib/Wire/utility/twi_rx.o ./gr_common/rx63n/interrupt_handlers.o ./gr_common/rx63n/reboot.o ./gr_common/rx63n/util.o ./gr_common/rx63n/vector_table.o ./gr_common/rx63n/reset_program.o \
./USB_Host/adk.o ./USB_Host/BTD.o ./USB_Host/BTHID.o ./USB_Host/cdcacm.o ./USB_Host/cdcftdi.o ./USB_Host/cdcprolific.o ./USB_Host/hid.o ./USB_Host/hidboot.o ./USB_Host/hidescriptorparser.o ./USB_Host/hiduniversal.o ./USB_Host/hwDmaIf.o ./USB_Host/masstorage.o ./USB_Host/message.o ./USB_Host/parsetools.o ./USB_Host/r_usbh_driver.o ./USB_Host/SPP.o ./USB_Host/Usb.o ./USB_Host/usbhBulk.o ./USB_Host/usbhControl.o ./USB_Host/usbhDriver.o ./USB_Host/usbhInterrupt.o ./USB_Host/usbhIsochronous.o ./USB_Host/usbhMain.o ./USB_Host/usbhPipe.o ./USB_Host/usbhub.o ./USB_Host/utilities/sysif.o \
./SSD1306Ascii/src/SSD1306Ascii.o
LIBFILES = ./gr_common/lib/DSP/utility/libGNU_RX_DSP_Little.a
CCINC = -I./gr_build -I./gr_common -I./gr_common/core -I./gr_common/core/avr -I./gr_common/lib -I./gr_common/lib/DSP -I./gr_common/lib/DSP/utility -I./gr_common/lib/EEPROM -I./gr_common/lib/EEPROM/utility -I./gr_common/lib/Firmata -I./gr_common/lib/LiquidCrystal -I./gr_common/lib/RTC -I./gr_common/lib/RTC/utility -I./gr_common/lib/SD -I./gr_common/lib/SD/utility -I./gr_common/lib/Servo -I./gr_common/lib/SoftwareSerial -I./gr_common/lib/SPI -I./gr_common/lib/Stepper -I./gr_common/lib/Wire -I./gr_common/lib/Wire/utility -I./gr_common/rx63n -I./USB_Driver \
-I./USB_Host -I./USB_Host/utilities \
-I./SSD1306Ascii/src/
HEADERFILES = ./gr_common/core/Arduino.h ./gr_common/core/binary.h ./gr_common/core/HardwareSerial.h ./gr_common/core/HardwareSerial_private.h ./gr_common/core/MsTimer2.h ./gr_common/core/new.h ./gr_common/core/pins_arduino.h ./gr_common/core/Print.h ./gr_common/core/Printable.h ./gr_common/core/Stream.h ./gr_common/core/Types.h ./gr_common/core/usbdescriptors.h ./gr_common/core/usb_cdc.h ./gr_common/core/usb_common.h ./gr_common/core/usb_core.h ./gr_common/core/usb_hal.h ./gr_common/core/utilities.h ./gr_common/core/WCharacter.h ./gr_common/core/wiring_private.h ./gr_common/core/WString.h ./gr_common/core/avr/avrlib.h ./gr_common/core/avr/pgmspace.h ./gr_common/lib/DSP/DSP.h ./gr_common/lib/DSP/utility/r_dsp_complex.h ./gr_common/lib/DSP/utility/r_dsp_filters.h ./gr_common/lib/DSP/utility/r_dsp_matrix.h ./gr_common/lib/DSP/utility/r_dsp_statistical.h ./gr_common/lib/DSP/utility/r_dsp_transform.h ./gr_common/lib/DSP/utility/r_dsp_typedefs.h ./gr_common/lib/DSP/utility/r_dsp_types.h ./gr_common/lib/EEPROM/EEPROM.h ./gr_common/lib/EEPROM/utility/r_flash_api_rx600.h ./gr_common/lib/Firmata/Boards.h ./gr_common/lib/Firmata/Firmata.h ./gr_common/lib/LiquidCrystal/LiquidCrystal.h ./gr_common/lib/RTC/RTC.h ./gr_common/lib/RTC/utility/RX63_RTC.h ./gr_common/lib/SD/SD.h ./gr_common/lib/SD/utility/FatStructs.h ./gr_common/lib/SD/utility/Sd2Card.h ./gr_common/lib/SD/utility/Sd2PinMap.h ./gr_common/lib/SD/utility/SdFat.h ./gr_common/lib/SD/utility/SdFatmainpage.h ./gr_common/lib/SD/utility/SdFatUtil.h ./gr_common/lib/SD/utility/SdInfo.h ./gr_common/lib/Servo/Servo.h ./gr_common/lib/SoftwareSerial/SoftwareSerial.h ./gr_common/lib/SPI/SPI.h ./gr_common/lib/Stepper/Stepper.h ./gr_common/lib/Wire/Wire.h ./gr_common/lib/Wire/utility/I2cMaster.h ./gr_common/lib/Wire/utility/twi_rx.h ./gr_common/rx63n/interrupt_handlers.h ./gr_common/rx63n/iodefine.h ./gr_common/rx63n/iodefine_gcc63n.h ./gr_common/rx63n/reboot.h ./gr_common/rx63n/rx63n_stdio.h ./gr_common/rx63n/specific_instructions.h ./gr_common/rx63n/typedefine.h ./gr_common/rx63n/user_interrupt.h ./gr_common/rx63n/util.h \
./USB_Host/address.h ./USB_Host/adk.h ./USB_Host/BTD.h ./USB_Host/BTHID.h ./USB_Host/cdcacm.h ./USB_Host/cdcftdi.h ./USB_Host/cdcprolific.h ./USB_Host/confdescparser.h ./USB_Host/hexdump.h ./USB_Host/hid.h ./USB_Host/hidboot.h ./USB_Host/hidescriptorparser.h ./USB_Host/hiduniversal.h ./USB_Host/hidusagestr.h ./USB_Host/hwDmaIf.h ./USB_Host/macros.h ./USB_Host/masstorage.h ./USB_Host/message.h ./USB_Host/parsetools.h ./USB_Host/printhex.h ./USB_Host/r_usbh_driver.h ./USB_Host/settings.h ./USB_Host/sink_parser.h ./USB_Host/SPP.h ./USB_Host/system_timer.h ./USB_Host/Usb.h ./USB_Host/usb110.h ./USB_Host/usbhConfig.h ./USB_Host/usbhDeviceApi.h ./USB_Host/usbhDriverInternal.h ./USB_Host/usbHost.h ./USB_Host/usbHostApi.h ./USB_Host/usbhost_typedefine.h ./USB_Host/usbhub.h ./USB_Host/usb_ch9.h ./USB_Host/utilities/ddusbh.h ./USB_Host/utilities/sysif.h 
TARGET = citrus_sketch
GNU_PATH := /usr/share/gnurx_v14.03_elf-1/
GCC_VERSION := 4.8-GNURX_v14.03
CFLAGS :=-Wall -I"$(GNU_PATH)rx-elf/include" -I. -I"$(GNU_PATH)lib/gcc/rx-elf/$(GCC_VERSION)/include" -I"$(GNU_PATH)rx-elf/include/c++/$(GCC_VERSION)/" -I"$(GNU_PATH)rx-elf/include/c++/$(GCC_VERSION)/rx-elf/64-bit-double/" -ffunction-sections -fno-function-cse -fsigned-char -fdata-sections -mno-balign -DTESTING=1 -DGRSAKURA -DARDUINO=100 -DCPPAPP -D__RX_LITTLE_ENDIAN__=1 -D__T4__ -O2 -flto -mlittle-endian-data -mcpu=rx600 -m64bit-doubles $(MRUBY_CFLAGS)
AFLAGS :=-Wall -I"$(GNU_PATH)rx-elf/include" -I. -I"$(GNU_PATH)lib/gcc/rx-elf/$(GCC_VERSION)/include" -I"$(GNU_PATH)rx-elf/include/c++/$(GCC_VERSION)/" -I"$(GNU_PATH)rx-elf/include/c++/$(GCC_VERSION)/rx-elf/64-bit-double/" -ffunction-sections -fno-function-cse -fsigned-char -fdata-sections -mno-balign -DTESTING=1 -DGRSAKURA -DARDUINO=100 -DCPPAPP -D__RX_LITTLE_ENDIAN__=1 -D__T4__ -O2 -flto -mlittle-endian-data -mcpu=rx600 -m64bit-doubles
SFLAGS :=--gdwarf2
CC  = rx-elf-gcc
CPP = rx-elf-gcc
AS  = rx-elf-as
LNK = rx-elf-ld
AR  = rx-elf-ar rcs
CNVS = rx-elf-objcopy -O srec -I elf32-rx-be-ns
CNVB = rx-elf-objcopy -O binary
DMP = rx-elf-objdump
OBJS = $(OBJFILES) $(LIBFILES)
AOBJS = $(filter-out ./gr_sketch.o, $(OBJFILES))
LFLAGS = -Map ./gr_build/$(TARGET).map -e_PowerON_Reset -T"./gr_common/linker_arduino.gsi" -no-keep-memory -S -L"$(GNU_PATH)rx-elf/lib/64-bit-double" -L"$(GNU_PATH)lib/gcc/rx-elf/$(GCC_VERSION)/64-bit-double" -L"$(GNU_PATH)rx-elf/lib/64-bit-double" -L"$(GNU_PATH)lib/gcc/rx-elf/$(GCC_VERSION)/64-bit-double" --no-flag-mismatch-warnings --start-group -lstdc++ -lnosys -lsim -lm -lc -lgcc $(MRUBY_LDFLAGS) $(MRUBY_LIBS) --end-group
MAKEFILE = makefile

make = make --no-print-directory

all:	rom

rom:	$(OBJS) $(MAKEFILE)
	$(LNK) $(OBJS) $(LFLAGS) -o ./gr_build/$(TARGET).x
	$(CNVB) ./gr_build/$(TARGET).x  $(TARGET).bin
	$(CNVS) ./gr_build/$(TARGET).x  ./gr_build/$(TARGET).mot
	rm -f *.o

%.o: %.s
	$(AS) $(SFLAGS) $(CCINC) $< -o $@

%.o: %.asm
	$(CC) $(AFLAGS) $(CCINC) -c -x assembler-with-cpp $< -o $@

%.o: %.c $(HEADERFILES)
	$(CC) $(CFLAGS) $(CCINC) -c -x c $< -o $@

%.o: %.cpp $(HEADERFILES)
	$(CPP) $(CFLAGS) $(CCINC) -c -x c++ $< -o $@

clean:
	rm -f $(OBJFILES)
	rm -f ./gr_build/$(TARGET).x
	rm -f $(TARGET).bin
	rm -f ./gr_build/$(TARGET).mot
	rm -f ./gr_build/$(TARGET).map

bin: core
	$(CNVB) $(TARGET).x  $(TARGET).bin

core: lib
	$(LNK) gr_sketch.o ./gr_common/rx63n/vector_table.o ./gr_common/rx63n/interrupt_handlers.o core.a $(LFLAGS) -o $(TARGET).x
	$(CNVS) $(TARGET).x  $(TARGET).mot

lib: $(AOBJS) $(MAKEFILE)
	$(AR) core.a $(AOBJS)
