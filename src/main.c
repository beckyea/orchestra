/* Name: main.c
 * Author: Becky Abramowitz
 */
#include "m_general.h"
#include "m_usb.h"
#include "m_rf.h"
#include "m_bus.h"
#include <stdio.h>

#define RX_ADDRESS 0x49
#define CHANNEL 1
#define PACKET_LENGTH 3


volatile uint8_t cnt;
volatile int sinTable[50];
volatile long frequency;
volatile char newSound;
volatile char stopSound;
char buffer[PACKET_LENGTH];
volatile uint8_t duration;

void init(void);
void createSinTable(void);
void gatherPacketData(void);

int main (void) {
	init();
	createSinTable();
	while (true) {
		OCR1B = sinTable[cnt] * OCR1A / 100;
		if (TCNT1 > OCR1B) { TCNT1 = 0; }
	}
	return 0;
}

void init(void) {
	// Initialize USB Communication
	m_usb_init();
	m_bus_init();
	sei();
	m_clockdivide(2);
	stopSound = 1;
	newSound = 0;
	set(DDRF, 1);
	m_rf_open(CHANNEL, RX_ADDRESS, PACKET_LENGTH);
	// Set Timer 0 for incrimenting the counter
	OCR0A = 100;
	clear(TCCR0B, CS02); clear(TCCR0B, CS01); set(TCCR0B, CS00); // set prescaler to /1
	clear(TCCR0B, WGM02); set(TCCR0A, WGM01); clear (TCCR0A, WGM00); // set to up to OCR0A mode
	set(DDRD, 0); // enable output on D0
	clear(TCCR0A, COM0B1); set(TCCR0A, COM0B0); //toggle when reaches OCR0A
	set(TIMSK0, OCIE0A); // enable interrupt
	// Set Timer 1 to enable the AC output
	OCR1A = 100;
	clear(TCCR1B, CS12); clear(TCCR1B, CS11); set(TCCR1B, CS10); // set prescaler to /1
	set(TCCR1B, WGM13); set(TCCR1B, WGM12); set(TCCR1A, WGM11); set(TCCR1A, WGM10);  // timer mode to 15
	//set(DDRB, 6); // set B6 to enable the comparison output
	set(TCCR1A, COM1B1); clear(TCCR1A, COM1B0);  // set at OCR1B, clear at rollover
	// Set Timer 3 to deal with duration
	OCR3B = 100;
	set(TCCR3B, CS32); clear(TCCR3B, CS31); clear(TCCR3B, CS30); // set clock prescalar to /64
	clear(TCCR3B, WGM33); set(TCCR3B, WGM32); clear(TCCR3A, WGM31); clear(TCCR3A, WGM30); // set to mode 4
	set(DDRC, 6); // set C6 to enable output
	set(TIMSK3, OCIE3A); // enable interrupt;

}

void createSinTable(void) {
	int i;
	for (i = 0; i < 50; i++) {
		sinTable[i] = 50+ 50 * (sin(2 * M_PI * i/50.0));
	}
}

void gatherPacketData(void) {
	m_rf_read(buffer, PACKET_LENGTH);
	frequency = (*(int*)&buffer[0]);
	duration = buffer[2]; // duration in centiseconds // 75 centiseconds
	OCR0A = 800000L/frequency;
}

ISR(TIMER0_COMPA_vect) {
	cnt = cnt + 1;
	if (cnt >= 50) { cnt = 0; }
}

ISR(INT2_vect) { 
	set(DDRB, 6);
	m_red(ON);
	set(PORTF, 1);
	gatherPacketData();
	OCR3A = duration * 80; 
	TCNT3 = 0;
}
ISR(TIMER3_COMPA_vect) { 
	clear(DDRB, 6);
	clear(PORTF, 1);
	m_red(OFF); 
}