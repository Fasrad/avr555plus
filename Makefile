CC=/usr/bin/avr-gcc
MEGA=328
CFLAGS=-Os -std=c99 -mmcu=atmega$(MEGA)
OBJ2HEX=/usr/bin/avr-objcopy 
PROG=/usr/bin/avrdude
TARGET=avr555plus

program : $(TARGET).hex
	$(PROG) -c usbasp -p m$(MEGA) -P /dev/ttyACM0 -e
	$(PROG) -c usbasp -p m$(MEGA) -P /dev/ttyACM0 -U flash:w:$(TARGET).hex

%.obj : %.o
	$(CC) $(CFLAGS) $< -o $@

%.hex : %.obj
	$(OBJ2HEX) -j .text -j .data -O ihex $< $@  

clean :
	rm *.hex *.obj *.out 
