/*
 * capstone_PwmAdcExinterrupt_test2.c
 *
 * Created: 2022-05-09 오후 5:00:48
 * Author : kimhyeonsoo
 */ 
 
	//OCR1A=(c<<1)+c+(c>>3);		//2+1+(1/8)=3.125
	//OCR1A=c+(c>>1)+(c>>4);		//1+(1/2)+(1/16)=1.5625
	//OCR1A=(c>>1)+(c>>2)+(c>>5);	//(1/2)+(1/4)+(1/32)=0.78125
	//OCR1A=(c>>2)+(c>>3)+(c>>6);	//(1/4)+(1/8)+(1/64)=0.390625

#define F_CPU 16000000UL
//////////// switching frequency selection ////////////
#define k_5				3.1250				// the ratio of ICR1 to c(ADCW) for 5kHz
#define k_10			1.5625				// the ratio of ICR1 to c(ADCW) for 10kHz
#define k_20			0.78125				// the ratio of ICR1 to c(ADCW) for 20kHz
#define k_40			0.390625			// the ratio of ICR1 to c(ADCW) for 40kHz
#define MaxCnt_5		3199
#define MaxCnt_10		1599
#define MaxCnt_20		799
#define MaxCnt_40		399
#define SpeedMaxCnt		62499

#define rotationCW		(0<<PORTA0)|(1<<PORTA1)		// StartStop->PORTA0	
#define rotationCCW		(0<<PORTA0)|(0<<PORTA1)		// DIR->PORTA1
#define rotationStop	(1<<PORTA0)
// Clear START/STROP in order to start the motor

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

void ExtInterrupt_init(void);
void Pwm_init(void);
void ADC_init(void);

volatile unsigned int c;
volatile unsigned int switchCnt=0;
volatile unsigned int timerFlag=0;
volatile unsigned int hallData;
volatile unsigned int hallTrigger;
volatile unsigned int encTrigger;
volatile unsigned int speedClock;
volatile unsigned int speedRPM;

ISR(TIMER1_OVF_vect)			// execution this loop every 0.2ms -> switching frequency=5kHz
{
	if(timerFlag==1){
		if(switchCnt==0){
			PORTA=rotationCW;
			
			ADCSRA|=(0x01<<ADSC);
			c=ADCW;
			OCR1A=c*k_20;
		}
		else if(switchCnt==1){
			PORTA=rotationCCW;
			
			ADCSRA|=(0x01<<ADSC);
			c=ADCW;
			OCR1A=c*k_20;
		}
		else;
	}
	else if(timerFlag==0){
		OCR1A=0;
		PORTA=rotationStop;
	}
	else;
}

ISR(INT0_vect){// toggle switch-related
	if(timerFlag==0){
		timerFlag=1;
	}
	else if(timerFlag==1){
		timerFlag=0;
	}
	else;
}
ISR(INT1_vect){// toggle switch-related
	if(PORTA==rotationStop){// only can change when the motor stop
		if(switchCnt==0){
			switchCnt=1;
		}
		else if(switchCnt==1){
			switchCnt=0;
		}
		else;
	}
	else;
}
//ISR(INT2_vect){// hall data processing-related
	////PORTF=~(PORTF0);
	//
	//if(hallTrigger==0){
		//hallTrigger=1;
		//TCCR3B=(1<<CS32)|(0<<CS31)|(0<<CS30);	// timer start(prescale:256)
		//TCNT3=0;
	//}
	//else if(hallTrigger==1){
		//hallTrigger=0;
		//speedClock=TCNT3+1;						// save a counter
		//TCCR3B=(0<<CS32)|(0<<CS31)|(0<<CS30);	// timer stop
		//speedRPM=180/speedClock;				//
		//PORTF=~(PORTF0);
	//}
	//else;
//}

int main(void)
{
	//* General Purpose I/O settings *//
	DDRA=(1<<DDA0)|(1<<DDA1);				// rotation settings
	PORTA=rotationStop;
	
	//* Timer I/O settings *//
	DDRB=(0x01<<DDB5);							// pwm output(using OC1A(PB5),OC1B(PB6))
	PORTB=0x00;	
	
	// CW/CCW check
	//DDRG=(1<<DDG0);
	//PORTG=0x00;
	
	
	//DDRE=(0x01<<DDE3);
	//PORTE=0x00;								// initialization
	
	//* debugging *//
	//DDRF=(1<<DDF0);
	//PORTF=0x00;
	
	//* Interrupt settings *//
	cli();						// entire interrupt Disable
	ExtInterrupt_init();
	Pwm_init();
	ADC_init();
	sei();						// entire interrupt Enable
	
    /* Replace with your application code */
    while (1) 
    {
    }
}

void ExtInterrupt_init(void)
{
	//EIMSK=(0x01<<INT0)|(0x01<<INT7)|(0x01<<INT6)|(0x01<<INT5);									// INT7:5 is Hall effect sensor output, INT0 is switch
	//EIMSK=(0x01<<INT2)|(0x01<<INT1)|(0x01<<INT0);									// INT7:5 is Hall effect sensor output, INT0 is switch
	//EICRA=(0x00<<ISC21)|(0x01<<ISC20)|(0x01<<ISC11)|(0x00<<ISC10)|(0x01<<ISC01)|(0x00<<ISC00);		// INT0:falling, INT1:falling, INT2:falling/rising
	EIMSK=(0x01<<INT1)|(0x01<<INT0);									// INT7:5 is Hall effect sensor output, INT0 is switch
	EICRA=(0x01<<ISC11)|(0x00<<ISC10)|(0x01<<ISC01)|(0x00<<ISC00);		// INT0:falling, INT1:falling, INT2:falling/rising															// falling edge trigger
	//EICRB=(0x00<<ISC71)|(0x01<<ISC70)|(0x00<<ISC61)|(0x01<<ISC60)|(0x00<<ISC51)|(0x01<<ISC50);	// falling or rising edge trigger
}

void Pwm_init(void)
{
	TCCR1A=(0x01<<COM1A1)|(0x00<<COM1A0)|(0x01<<WGM11)|(0x00<<WGM10);			// NoneINV mode
	TCCR1B=(0x01<<WGM13)|(0x01<<WGM12)|(0x00<<CS12)|(0x00<<CS11)|(0x01<<CS10);	// prescale:1	WGM13:10=1110(Fast PWM)
	TCCR1C=0x00;
	TCNT1=0;
	ICR1=MaxCnt_20;		// 799 : 20kHz
	TIMSK=(0x01<<TOIE1);	// overflow interrupt enable
	
	//TCCR3A=(0x01<<COM3A1)|(0x00<<COM3A0)|(0x00<<WGM11)|(0x00<<WGM10);			// Normal mode
	//TCCR3B=(0x00<<WGM13)|(0x00<<WGM12)|(0x00<<CS12)|(0x00<<CS11)|(0x00<<CS10);	// Don't start yet
	//TCCR3C=0x00;
	//TCNT3=0;
	//ICR3=SpeedMaxCnt;
	//ETIMSK=(0x01<<TOIE3);
	
	//TCCR3A=(0x01<<COM3A1)|(0x00<<COM3A0)|(0x01<<WGM11)|(0x00<<WGM10);			// NoneINV mode
	//TCCR3B=(0x01<<WGM13)|(0x01<<WGM12)|(0x00<<CS12)|(0x00<<CS11)|(0x01<<CS10);	// prescale:1	WGM13:10=1110(Fast PWM)
	//TCCR3C=0x00;
	//TCNT3=0;
	//ICR3=MaxCnt_20;
	//ETIMSK=(0x01<<TOIE3);
}

void ADC_init(void)
{
	ADMUX=(0x00<<REFS1)|(0x01<<REFS0)|(0x03<<MUX0);															//   ADC0, power supply
	//ADMUX=(0x00<<REFS1)|(0x00<<REFS0)|(0x00<<MUX0);															//   ADC0, adapter
	ADCSRA=(0x01<<ADEN)|(0x00<<ADSC)|(0x00<<ADFR)|(0x01<<ADPS2)|(0x00<<ADPS1)|(0x00<<ADPS0);
}