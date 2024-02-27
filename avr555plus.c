/*-------|---------|---------|---------|---------|---------|---------|---------|
avr555plus.c	


by chaz miller 

HARDWARE:
for ATMEGA328 set at 1MHz running at 5V. 

This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3 or any later
version. This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
******************************************************************/

#include <avr/io.h>
#define boil_setpoint 0x980    //legacy
#define hyst 2    //legacy. 1/10th deg. C per count at 12 bits; optional

void delay(uint16_t); // generic delay function 
void blink (uint8_t); // blinks an LED for diagnostic
void adc_init(void);  // Inits ADC at 12 bits oversampled 
/* adc_read takes PORTC bitmask. Returns 12 bit value. Blocking; not tested to be 
interrupt-safe. Depends on clock setups. ADC*/
uint16_t adc_read(uint8_t); 

int main(){
    //8 bit Timer 0 is used by delay(). PWM on PD6 and PD5. 
    TCCR0A |= 1<<WGM00;         //phase correct top=OCRA; divides by 2
    TCCR0B |= 1<<WGM02;

    TCCR0B |= 1<<CS00;           //fcpu / 1 (1MHz)
    //TCCR0B |= 1<<CS01;           //fcpu / 1 (1MHz)
    TCCR0B = 1<<CS02;           //fcpu / 1 (1MHz)
    //TCCR0B = 1<<CS02;           //fcpu / 1 (1MHz)
    OCR0A  = 100;               //sets TOP; should be 1kHz
    OCR0B  = 50;                //50% duty cycle, pwm on PD5

    //16-bit Timer 1 used as output PWM on OC1B PB2 p.115
    //noninverting phase correct, CTC-PWM hybrid mode p135 
    TCCR1A = (1<<COM1B1)|(1<<WGM11)|(1<<WGM10); 
    TCCR1B = (1<<WGM13)|(1<<CS12);                 //  clk/256
    //TCCR1B = (1<<WGM13)|(1<<CS11)|(1<<CS10);       //  clk/64
    //TCCR1B = (1<<WGM13)|(1<<CS10);                 //  clk/1
    OCR1A = 0x0FF0;             //sets pwm TOP value
    OCR1B = 0;
    PORTB = 0xFF;
    DDRB = 0b000100100;        //LED on PB5; OC1B is PB2
    DDRD = 0xFF;        //output 
    adc_init();
    /****************************************
    *****main loop***************************
    ****************************************/
    // this is where we do hardware detection.

    //this is where we call functions in other source files like program0, program1 etc., possibly with a switch statement?

    for(;;){  
    blink(20);
    delay(500);
    } //infty
}//main

void adc_init(void){
    //ADCSRA = (1<<ADEN)|(1<<ADPS1)|(1<<ADPS0); //125kHz @ 1MHz clock, page 264
    ADCSRA = (1<<ADEN)|(1<<ADPS2); //62kHz @ 1MHz clock, page 264
    ADMUX |= (1<<REFS0);           //Avcc (5v)
    //ADMUX |= (1<<ADLAR);           //left align for 8-bit operation
    ADCSRA |= (1<<ADEN);  // redundant?? Some you have to write twice...
    //ADCSRA |= (1<<ADSC);  
}

uint16_t adc_read(uint8_t me){    //expects register value, not port pin label
    uint16_t ad_bucket=0;
    ADMUX &= 0xF0;
    ADMUX |= me;
    for (int i=0; i<16; i++){
	ADCSRA |= (1<<ADSC); 
	while(ADCSRA & (1<<ADSC)); 
	ad_bucket += ADCW;
    }
    return (ad_bucket>>2); //12 bits oversampled
}

void delay(uint16_t me){    //at 1MHz, each unit is 2.55us. 1ms is 4units. 
    while(me){              //interferes with PWM timer; for debug
	while(TCNT0 < 128){}
	me--;
	while(TCNT0 > 128){}
    }
}

void blink (uint8_t me){
    for (int i=0; i<me; i++){
	PORTB |= (1<<5);
	delay(20);
	PORTB &= ~(1<<5);
	delay(200);
    }
    delay(500);
}
/*Set a bit
 bit_fld |= (1 << n)

Clear a bit
 bit_fld &= ~(1 << n)

Toggle a bit
 bit_fld ^= (1 << n)

Test a bit
 bit_fld & (1 << n)
*/ 
