# Makefile für Arduino Mega 2560 Firmware, Aufruf mit "nmake" oder "jom", April 2025

AS7PATH   = C:\uti\as70

TOOLPATH  = $(AS7PATH)\toolchain\avr8\avr8-gnu-toolchain
DFPPATH   = $(AS7PATH)\Packs\atmel\ATmega_DFP\1.7.374
BINDIR    = $(TOOLPATH)\bin

PROJECT = wetterstation

DEVICE  = atmega2560

# Atmel DFP Packs
DFPINCLUDE= $(DFPPATH)\include
DFPLIB    = $(DFPPATH)\gcc\dev\$(DEVICE)

CFLAGS = -x c -funsigned-char -funsigned-bitfields -DNDEBUG \
         -I $(DFPINCLUDE) \
         -Os \
         -c -std=gnu99 \
         -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -mrelax -Wall \
         -mmcu=$(DEVICE)

LFLAGS = -Wl,--start-group -Wl,-lm  -Wl,--end-group -Wl,--gc-sections -mrelax \
         -mmcu=$(DEVICE) \
         -B $(DFPLIB)

OFLAGS  = -Os

CC      = $(BINDIR)\avr-gcc.exe
OBJCOPY = $(BINDIR)\avr-objcopy.exe
AVRSIZE = $(BINDIR)\avr-size.exe

$(PROJECT).hex: $(PROJECT).elf
        $(OBJCOPY) -O ihex  -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures   $(PROJECT).elf  $(PROJECT).hex
        $(AVRSIZE) $(PROJECT).elf

$(PROJECT).elf: main.o platform.o bmp280.o i2clcd.o softi2c.o keypad.o rtc.o
        $(CC) $(LFLAGS) -Wl,-Map,$(PROJECT).map  \
        main.o \
        platform.o \
        keypad.o \
        rtc.o \
        bmp280.o   \
        i2clcd.o   \
        softi2c.o  \
        -o $(PROJECT).elf

main.o: Makefile main.c platform.h
        $(CC) $(CFLAGS) $(OFLAGS)  -c main.c

platform.o: Makefile platform.c platform.h
        $(CC) $(CFLAGS) $(OFLAGS)  -c platform.c

rtc.o: Makefile rtc.c platform.h
        $(CC) $(CFLAGS) $(OFLAGS)  -c rtc.c

keypad.o: Makefile keypad.c platform.h
        $(CC) $(CFLAGS) $(OFLAGS)  -c keypad.c

bmp280.o: Makefile bmp280.c platform.h
        $(CC) $(CFLAGS) $(OFLAGS)  -c bmp280.c

i2clcd.o: Makefile i2clcd.c platform.h
        $(CC) $(CFLAGS) $(OFLAGS)  -c i2clcd.c

softi2c.o: Makefile softi2c.c platform.h
        $(CC) $(CFLAGS) $(OFLAGS)  -c softi2c.c

clean:
        del *.o

