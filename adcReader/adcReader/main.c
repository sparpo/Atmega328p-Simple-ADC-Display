/*
*
* Adc reader
* Created: 24/02/2020 13:29:15
* Author : Joey Corbett
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#define adc_max 1024.0

#define Vref 5000

#define delayTime 40

#define readings 5

void sendmsg (char *s);
void init_USART(void);
void init_ADC(void);
void init_ports(void);
void init_timer0(void);

/* timer0 variables */
unsigned int timecount0;


/* ADC variables  */
unsigned long int adc_mV;
unsigned long int adc_reading[readings];
unsigned long int adc_average;
volatile unsigned int new_adc_data;

/* UART variables */
unsigned char qcntr = 0,sndcntr = 0;   /*indexes into the queue*/
unsigned char queue[50];       /*character queue*/
char data[30];
char ch;  /* character variable for received character*/

void init_ADC() {
	ADMUX = (1<<REFS0) | (3<<MUX0); //sets voltage ref
	ADCSRA  = 0b11101111; //enable adc, starts conversion, enable interrupt, sets prescalar 128
	ADCSRB = 0;// sets free running mode
}

/* Initializing USART registers */
void init_USART() {
	UCSR0B	= (1<<RXEN0) | (1<<TXEN0) | (1<<TXCIE0) | (0<<UCSZ02);  //enable receiver, transmitter, TX Complete and transmit interrupt and setting data to 8 bits
	UBRR0 = 16;  //baud rate = 58823, 57600
}
void init_ports() {
	
	DDRB = (1<<5);		// Initialize PB5 to output (in-built led on arduino board)
	PORTB = 1<<5;		// Turn built-in led on, to make sure board is working
}
void init_timer0() {
	timecount0 = 0;		/* Initialise the overflow count. Note its scope	*/
	TCCR0B = (5<<CS00);	/* Set T0 Source = Clock (16MHz)/1024 and put Timer in Normal mode	*/
	TCCR0A = 0;			/* Not strictly necessary as these are the reset states but it's good	*/
	TCNT0 = 61;			/* Recall: 256-61 = 195 & 195*64us = 12.48ms, approx 12.5ms		*/
	TIMSK0 = (1<<TOIE0);		/* Enable Timer 0 interrupt										*/
	
}


int main(void) {
	init_ADC();
	init_USART();
	init_ports();
	init_timer0();
	sei();						/* Global interrupt enable (I=1)								*/

	while(1){
		if (new_adc_data) {
			/*adc_mV = (adc_average/adc_max)*Vref; // Calculates ADC in mV
			sprintf(data, "ADC value = %lu mV",adc_mV); //Report ADC value in mV
			sendmsg(data);*/
			new_adc_data = 0;
			int total = 0; //avg
			for(int i = 0;i<readings;i++) {
				total+=adc_reading[i];
			}
			adc_average = total/readings; //avg adc reading
			if(adc_average>=4) {
				PORTB = (1<<5);
			} else {
				PORTB = 0;
			}
		}
		if (UCSR0A & (1<<RXC0)) {/*check for character received*/
			
			ch = UDR0;    /*get character sent from PC*/
			switch (ch) { //character input

				/* Report ADC Value to user */
				case 'A':
				case 'a':
				sprintf(data, "ADC value = %lu", adc_average); //Report ADC value
				sendmsg(data);
				break;
				
				/* Report ADC Value in mV to user */
				case 'V':
				case 'v':
				adc_mV = (adc_average/adc_max)*Vref; // Calculates ADC in mV
				sprintf(data, "ADC value = %lu mV",adc_mV); //Report ADC value in mV
				sendmsg(data);
				break;
			}
		}
	}
}
/* sendmsg function*/
void sendmsg (char *s) {
	qcntr = 0;    /*preset indices*/
	sndcntr = 1;  /*set to one because first character already sent*/
	
	queue[qcntr++] = 0x0d;   /*put CRLF into the queue first*/
	queue[qcntr++] = 0x0a;
	while (*s)
	queue[qcntr++] = *s++;   /*put characters into queue*/
	
	UDR0 = queue[0];  /*send first character to start process*/
}

/********************************************************************************/
/* Interrupt Service Routines													*/
/********************************************************************************/

/*this interrupt occurs whenever the */
/*USART has completed sending a character*/

ISR(USART_TX_vect) {
	/*send next character and increment index*/
	if (qcntr != sndcntr)
	UDR0 = queue[sndcntr++];
}
ISR (ADC_vect) {//handles ADC interrupts
	static unsigned int adccount;
	new_adc_data = 1;
	if(adccount < readings) {
		adc_reading[adccount] = ADC;   /* ADC is in Free Running Mode - you don't have to set up anything for
		the next conversion */
		adccount++;
		
		} else {

		adccount = 0;
	}
	
}
ISR(TIMER0_OVF_vect){
	/*adc_mV = (adc_average/adc_max)*Vref; // Calculates ADC in mV
	sprintf(data, "ADC value = %lu mV",adc_mV); //Report ADC value in mV
	sendmsg(data);*/
	TCNT0 = 61;		/*	TCNT0 needs to be set to the start point each time				*/
	++timecount0;			/* count the number of times the interrupt has been 	reached			*/
		adc_mV = (adc_average/adc_max)*Vref; // Calculates ADC in mV
		sprintf(data, "%lumV",adc_mV); //Report ADC value in mV
		sendmsg(data);
	if (timecount0 >= delayTime){		

		//PORTB = PORTB ^ (1<<5);
		timecount0 = 0;		/* Restart the overflow counter					*/
	}
	

	
}

