/*-------|---------|---------|---------|---------|---------|---------|---------|
bottomStepper.c	
For the bottom half of panoramicam. Written for the ATMEGA 168 microcontroller
and avr-gcc compiler.
This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3 or any later
version. This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
******************************************************************/

#define mdelay 5         //sets manual positioning speed; see delay()
#define F_CPU 16000000UL //*16MHz*
#include <avr/io.h>

void delay(uint16_t);
void die (uint8_t);
void blink (uint8_t);
void ustep(uint8_t);

uint16_t dly;    //delay between steps, in timer ticks

int main(){
    //set up port pins 
    DDRB |= 0b00101000;
    DDRD |= 0b01101000;
    DDRC = 0;
    PORTB = 0xFF;
    PORTC = 0xFF;
    PORTD = 0xFF;
    //PWM timer configs
    TCCR0A = 0b10100011;     //fastPWM;  p.103 in datasheet
    TCCR2A = 0b10100011;     //fastPWM;  p.153
    TCCR0B = 2;              //F_CPU/8;  p.105  (7.84 kHz)
    TCCR2B = 2;              //F_CPU/8;  p.157
    OCR0A = 128;             //gently lock stepper rotation 
    //startup blinkenled
    for (int i=0; i<8; i++){
	PORTB |= (1<<5);
	delay(800);
	PORTB &= ~(1<<5);
	delay(800);
    }
    delay(8000);

    //read in DIPswitch; set up TIMER1 per external calculation 
    switch (PINC & 0b00000111){
	case 0:
	    dly=3333;             //8s revolution
	    TCCR1B |= (1);        //F_CPU/1; page 133; 
	    blink (0);
	    break;
	case 1:
	    dly=6666;             //16s revolution
	    TCCR1B |= (1);        //F_CPU/1
	    blink (1);
	    break;
	case 2:
	    dly=1667;             //32s revolution
	    TCCR1B |= (2);     //F_CPU/8
	    blink (2);
	    break;
	case 3:
	    dly=3333;             //64s revolution
	    TCCR1B |= (2);     //F_CPU/8
	    blink (3);
	    break;
	case 4:
	    dly=3906;             //10min revolution
	    TCCR1B |= (3);     //F_CPU/64
	    blink (4);
	    break;
	case 5:
	    dly=23437;            //1hr revolution
	    TCCR1B |= (3);     //F_CPU/64
	    blink (5);
	    break;
	case 6:
	    dly=2929;             //2hr revolution
	    TCCR1B |= (5);        //F_CPU/1024
	    blink (6);
	    break;
	case 7:
	    dly=5859;            //4hr revolution
	    TCCR1B |= (5);        //F_CPU/1024
	    blink (7);
	    break;
	default:
	    die (1);
    }

    while(PIND & 1<<1){           //allow user to pre-position camera
	if(!(PIND & 1<<2)){
	   ustep(1);
	} else if(!(PIND & 1)){
	   ustep(2);
	}
	delay(mdelay);
    }
    //button has been pressed; initiate pictionation 

    blink(10);

    TCNT1 = 0;

    while(1){
	if (TCNT1 >= dly){die (2);}   //catch possible timer underrun
	while(TCNT1 < dly){}          //poll timer 
	TCNT1 = 0;          
	ustep(2);
    }
}//main

void ustep(uint8_t me){
    /********************************************************
    Sinewave output routine for unipolar stepper motor
    1 is clockwise and 2 is anticlockwise 
    *********************************************************/
    /*
    //0--actual sine wave 
    static uint8_t sinewave[64]= {0,13,25,37,50,62,74,86,98,109,120,131,
    142,152,162,171,180,189,197,205,212,219,225,231,236,240,244,247,250,252,
    254,255,255,255,254,252,250,247,244,240,236,231,225,219,212,205,197,189,
    180,171,162,152,142,131,120,109,98,86,74,62,50,37,25,13};
    //1--semicircle
    static uint8_t sinewave[64]= {0,59,86,106,122,136,148,158,168,177,185,
    192,199,205,210,216,221,225,229,233,236,239,242,245,247,249,250,252,253,
    254,254,255,254,254,253,252,250,249,247,245,242,239,236,233,229,225,221,
    216,210,205,199,192,185,177,168,158,148,136,122,106,86,59};
    //2--semicircle with top chopped off
    static uint8_t sinewave[64]= {68,99,122,140,156,170,182,193,204,213,221,
    229,236,242,248,254,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,254,248,
    242,236,229,221, 213,204,193,182,170,156,140,122,99,68,0};
    //3--semicircle with sides chopped off 
    static uint8_t sinewave[64]={116,129,140,151,160,168,176,183,190,196,
    201,207,212,216,220,224,228,231,234,237,240,242,244,246,248,250,251,252,
    253,254,254,254,255,254,254,254,253,252,251,250,248,246,244,242,240,237,
    234,231,228,224,220,216,212,207,201,196,190,183,176,168,160,151,140,129};
    //4--overlapping, half-steppy radically top-chopped
    static uint8_t sinewave[64]={156,169,180,191,200,208,216,223,230,236,243,
    248,252,254,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,254,252,248,243,236,230,223,216,208,200,191,180,169};
    //5--center-dipped
    static uint8_t sinewave[64]={156,169,180,191,200,208,216,223,230,236,243,
    248,252,254,255,255,255,254,254,253,253,252,251,250,250,249,249,248,247,
    247,246,246,245,246,246,247,247,248,249,249,250,250,251,252,253,253,254,
    254,255,255,255,254,252,248,243,236,230,223,216,208,200,191,180,169};
    */
    //6--less overlap; center-dipped
    static uint8_t sinewave[64]={50,75,99,122,140,156,170,185,197,210,220,227,
    235,242,248,252,254,255,255,255,254,253,252,251,250,249,248,247,246,245,
    244,244,244,245,246,247,248,249,250,251,252,253,254,255,255,255,255,255,
    254,250,248,245,240,233,225,212,200,188,175,160,145,130,104,68};

    static uint8_t state; 
    //I use the most significant 3 bits of byte 'state' to track the 
    //output pin state table; the lower 5 bits track the PWM duty cycle

    //increment the duty cycles to microstep the motor
    if (2==me){
	state--;
    } else if (1==me){
	state++; 
    } else {die (5);}

    //write out the PWM compare register values
    switch ((state>>5)&3){ 
	case 0:
	    OCR0A = sinewave[(state+32)&63];
	    OCR0B = sinewave[state&63];
	    OCR2A = 0;
	    OCR2B = 0;
	    break;
	case 1:
	    OCR0A = 0;
	    OCR0B = sinewave[state&63];
	    OCR2A = sinewave[(state+32)&63];
	    OCR2B = 0;
	    break;
	case 2:
	    OCR0A = 0;
	    OCR0B = 0;
	    OCR2A = sinewave[(state+32)&63];
	    OCR2B = sinewave[state&63];
	    break;
	case 3:
	    OCR0A = sinewave[(state+32)&63];
	    OCR0B = 0;
	    OCR2A = 0;
	    OCR2B = sinewave[state&63];
	    break;
	default: 
	    die (3);
    }
}//ustep

void delay(uint16_t me){    //at F_CPU/8, each unit is 128us. 1ms is 8units. 
    while(me){
	while(TCNT0 < 128){}
	me--;
	while(TCNT0 > 128){}
    }
}

void die (uint8_t me){
    while(1){
	blink(me);
	delay(20000);
    }
}

void blink (uint8_t me){
    for (int i=0; i<me; i++){
	PORTB |= (1<<5);
	delay(3000);
	PORTB &= ~(1<<5);
	delay(3000);
    }
    delay(600);
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
