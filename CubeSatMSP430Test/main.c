/***************************************************************************************************
 *	        Include section					                       		   					       *
 ***************************************************************************************************/

#include <msp430.h>

#include <stdio.h>
/***************************************************************************************************
 *	        Prototype section					                       							   *
 ***************************************************************************************************/

//sensor data
void send_systemdata(int data);
void dac_write(char data);
unsigned int measure_adc(void);
signed int ResistanceToVoltage(int Resistance);
signed int ResistanceToCurrent(int Resistance);
int calculate_voltage(unsigned int adc_voltage_value);
int calculate_current(unsigned int adc_voltage_value);

/***************************************************************************************************
 *	        Global Variable section  				                            				   *
 ***************************************************************************************************/

//Flags
int send_data = 0;

//Counter
int counter = 0;

//Sensors
int model_output = 0;
unsigned int adc_voltage_value = 0;
unsigned int adc_current_value = 0;

//DAC
char sin[32] = {0x80, 0x99, 0xB2, 0xC8,//32 point sin wave sampled via MATLAB
 0xDC, 0xEC, 0xF7, 0xFE,
 0xFF, 0xFB, 0xF2, 0xE4,
 0xD3, 0xBD, 0xA6, 0x8C,
 0x73, 0x59, 0x42, 0x2C,
 0x1B, 0x0D, 0x04, 0x00,
 0x01, 0x08, 0x13, 0x23,
 0x37, 0x4D, 0x66, 0x7F};

/***************************************************************************************************
 *	        Define section					                       		   					       *
 ***************************************************************************************************/
#define LED_0 BIT0
#define LED_OUT P1OUT
#define LED_DIR P1DIR

/***************************************************************************************************
 *         Main section                                                                            *
 ***************************************************************************************************/

void main(void)
	{
	//Stops Watchdog
	WDTCTL = WDTPW | WDTHOLD;

	//Calibrate timer
	if(CALBC1_16MHZ==0xFF)
	{
		while(1);
	}
	DCOCTL = 0;
	BCSCTL1 = CALBC1_16MHZ;
	DCOCTL = CALDCO_16MHZ;

	//Timer 0
	TA0CCTL0 = CCIE;
	TA0CCR0 = 32768;
	TA0CTL = TASSEL_1 + MC_1;

    //Enable red LED
    LED_DIR = LED_0; //LEDs are set as output
    LED_OUT &= ~(LED_0); //LEDs are turned off

//    //Enable ports for UART connection
//	P1SEL  |= BIT1 + BIT2;	 	//Enable module function on pin
//	P1SEL2 |= BIT1 + BIT2;  	//Select second module; P1.1 = RXD, P1.2=TXD

	//Enable clock
	UCA0CTL1 |= UCSSEL_2;    	// Set clock source SMCLK
	UCA0BR0 = 104;           	// 1MHz 9600 (N=BRCLK/baudrate)
	UCA0BR1 = 0;             	// 1MHz 9600
	UCA0MCTL = UCBRS0;       	// Modulation UCBRSx = 1
	UCA0CTL1 &= ~UCSWRST;    	// **Initialize USCI state machine**

//	IE2 |= UCA0RXIE;         	//enable RX interrupt

	__enable_interrupt();

	while(42)
	{
		__bis_SR_register(GIE);

		if(send_data)
		{
//			//start bit (255)
//			model_output = 0xFF;
//			send_systemdata((int) (model_output));

			//temperature value (0..40)
			ADC10CTL0 = ADC10SHT_2 + ADC10ON; //64 clocks for sample
			ADC10CTL1 = INCH_4;	// Temp Sensor is at channel
			adc_voltage_value = measure_adc();
			model_output = calculate_voltage(adc_voltage_value);
//			send_systemdata((int) (model_output));
//saab MSC funktsiooni kasutada et s‰‰sta ressursi
			ADC10CTL0 = ADC10SHT_2 + ADC10ON; //64 clocks for sample
			ADC10CTL1 = INCH_5;	// Temp Sensor is at channel
			adc_current_value = measure_adc();
			model_output = calculate_current(adc_current_value);
//			send_systemdata((int) (model_output));

			for (x = 0; x < 32; x++)
			dac_write(sin[x]);

			P1OUT ^= LED_0;
			//exit send_data
			send_data = 0;
		}
	}
}

//#include <msp430x22x2.h>
//#include <UF_DAC.h>
//char sin[32] = {0x80, 0x99, 0xB2, 0xC8,//32 point sin wave sampled via MATLAB
// 0xDC, 0xEC, 0xF7, 0xFE,
// 0xFF, 0xFB, 0xF2, 0xE4,
// 0xD3, 0xBD, 0xA6, 0x8C,
// 0x73, 0x59, 0x42, 0x2C,
// 0x1B, 0x0D, 0x04, 0x00,
// 0x01, 0x08, 0x13, 0x23,
// 0x37, 0x4D, 0x66, 0x7F};
//void main(void){
//char x;
//WDTCTL = WDTPW + WDTHOLD;
//DCOCTL = CALDCO_16MHZ;
//BCSCTL1 = CALBC1_16MHZ;
//while(42){
//for (x = 0; x < 32; x++){
//dac_write(sin[x]);
//}
//}
//}

/* END: main */

void dac_write(char data){
int output = 0x0000;
char temp;
signed char x;
P2SEL &= 0xF8; //select digital I/0
P2DIR |= 0x07; //set directon register P2.0 - P2.2
P2OUT &= 0xF8; // clear data, clock, and enable
P2OUT |= 0x04; // set enable high
temp = data; // format data
data >>= 4;
data &= 0x0F;
data |= 0xF0;
output = data;
output <<= 8;
data = temp;
data <<= 4;
data &= 0xF0;
output |= data;
P2OUT &= 0xFB; //set enable low
for (x = 15; x > -1; x--){ //send out 16 bits of data
P2OUT |= (output >> x) & 0x01;
P2OUT |= 0x02;
P2OUT &= 0xFD;
P2OUT &= 0xFE;
}
P2OUT |= 0x04; // set enable high
}

unsigned int measure_adc(void)
{
	unsigned int raw_value = 0;
	ADC10CTL0 |= ENC + ADC10SC;	// Sampling and conversion start
	while(ADC10CTL1 & BUSY); // Wait..i am converting...
	raw_value = ADC10MEM;	// Read ADC memory
	return raw_value;
}

int calculate_voltage(unsigned int adc_voltage_value)
{
//	int Voltage = 0;
	int Resistance = 0;
	int Temperature = 0;

//	Voltage = ((1023-adc_voltage_value)*3.6)/1023; // Convert 256-bit value to voltage
	Resistance = ((10000*3.6/adc_voltage_value)); // Convert output voltage to resistance
	Temperature = ResistanceToVoltage(Resistance);
	return Temperature;
}

int calculate_current(unsigned int adc_current_value)
{
//	int Voltage = 0;
	int Resistance = 0;
	int Temperature = 0;

//	Voltage = ((1023-adc_voltage_value)*3.6)/1023; // Convert 256-bit value to voltage
	Resistance = ((10000*3.6/adc_current_value)); // Convert output voltage to resistance
	Temperature = ResistanceToCurrent(Resistance);
	return Temperature;
}

signed int ResistanceToVoltage(int Resistance)
{
	signed int termo=0;

	if(Resistance>31308)
	{
		termo=0;
	}
	else if ((Resistance<=31308)&&(Resistance>29749))
	{
		termo=1;
	}
	else if ((Resistance<=29749)&&(Resistance>28276))
	{
		termo=2;
	}
	else if ((Resistance<=28276)&&(Resistance>26885))
	{
		termo=3;
	}
	else if ((Resistance<=26885)&&(Resistance>25570))
	{
		termo=4;
	}
	else if ((Resistance<=25570)&&(Resistance>24327))
	{
		termo=5;
	}
	else if ((Resistance<=24327)&&(Resistance>23152))
	{
		termo=6;
	}
	else if ((Resistance<=23152)&&(Resistance>22141))
	{
		termo=7;
	}
	else if ((Resistance<=22141)&&(Resistance>20989))
	{
		termo=8;
	}
	else if ((Resistance<=20989)&&(Resistance>19993))
	{
		termo=9;
	}
	else if ((Resistance<=19993)&&(Resistance>19051))
	{
		termo=10;
	}
	else if ((Resistance<=19051)&&(Resistance>18158))
	{
		termo=11;
	}
	else if ((Resistance<=18158)&&(Resistance>17312))
	{ //20-25C
		termo=12;
	}
	else if ((Resistance<=17312)&&(Resistance>16511))
	{ //25-30C
		termo=13;
	}
	else if ((Resistance<=16511)&&(Resistance>15751))
	{ //30-40C
		termo=14;
	}
	else if ((Resistance<=15751)&&(Resistance>15031))
	{ //40-50C
		termo=15;
	}
	else if ((Resistance<=15031)&&(Resistance>14347))
	{ //50-60C
		termo=16;
	}
	else if ((Resistance<=14347)&&(Resistance>13698))
	{ //60-70C
		termo=17;
	}
	else if ((Resistance<=13698)&&(Resistance>13083))
	{ //70-80C
		termo=18;
	}
	else if ((Resistance<=13083)&&(Resistance>12499))
	{ //80-90C
		termo=19;
	}
	else if ((Resistance<=12499)&&(Resistance>11944))
	{ //80-90C
		termo=20; //I don't care if it's above 91 either
	}
	else if ((Resistance<=11944)&&(Resistance>11417))
	{
		termo=21;
	}
	else if ((Resistance<=11417)&&(Resistance>10916))
	{ //20-25C
		termo=22;
	}
	else if ((Resistance<=10916)&&(Resistance>10440))
	{ //25-30C
		termo=23;
	}
	else if ((Resistance<=10440)&&(Resistance>10000))
	{ //30-40C
		termo=24;
	}
	else if ((Resistance<=10000)&&(Resistance>9556))
	{ //40-50C
		termo=25;
	}
	else if ((Resistance<=9556)&&(Resistance>9147))
	{ //50-60C
		termo=26;
	}
	else if ((Resistance<=9147)&&(Resistance>8387))
	{ //60-70C
		termo=27;
	}
	else if ((Resistance<=8758)&&(Resistance>8387))
	{ //70-80C
		termo=28;
	}
	else if ((Resistance<=8387)&&(Resistance>8034))
	{ //80-90C
		termo=29;
	}
	else if ((Resistance<=8034)&&(Resistance>7700))
	{ //80-90C
		termo=30; //I don't care if it's above 91 either
	}
	else if ((Resistance<=7700)&&(Resistance>7377))
	{
		termo=31;
	}
	else if ((Resistance<=7377)&&(Resistance>7072))
	{ //20-25C
		termo=32;
	}
	else if ((Resistance<=7072)&&(Resistance>6781))
	{ //25-30C
		termo=33;
	}
	else if ((Resistance<=6781)&&(Resistance>6504))
	{ //30-40C
		termo=34;
	}
	else if ((Resistance<=6504)&&(Resistance>6239))
	{ //40-50C
		termo=35;
	}
	else if ((Resistance<=6239)&&(Resistance>5987))
	{ //50-60C
		termo=36;
	}
	else if ((Resistance<=5987)&&(Resistance>5746))
	{ //60-70C
		termo=37;
	}
	else if ((Resistance<=5746)&&(Resistance>5517))
	{ //70-80C
		termo=38;
	}
	else if ((Resistance<=5517)&&(Resistance>5298))
	{ //80-90C
		termo=39;
	}
	else if (Resistance<5298)
	{ //80-90C
		termo=40; //I don't care if it's above 91 either
	}
	 return termo;
}

signed int ResistanceToCurrent(int Resistance)
{
	signed int termo=0;

	if(Resistance>31308)
	{
		termo=0;
	}
	else if ((Resistance<=31308)&&(Resistance>29749))
	{
		termo=1;
	}
	else if ((Resistance<=29749)&&(Resistance>28276))
	{
		termo=2;
	}
	else if ((Resistance<=28276)&&(Resistance>26885))
	{
		termo=3;
	}
	else if ((Resistance<=26885)&&(Resistance>25570))
	{
		termo=4;
	}
	else if ((Resistance<=25570)&&(Resistance>24327))
	{
		termo=5;
	}
	else if ((Resistance<=24327)&&(Resistance>23152))
	{
		termo=6;
	}
	else if ((Resistance<=23152)&&(Resistance>22141))
	{
		termo=7;
	}
	else if ((Resistance<=22141)&&(Resistance>20989))
	{
		termo=8;
	}
	else if ((Resistance<=20989)&&(Resistance>19993))
	{
		termo=9;
	}
	else if ((Resistance<=19993)&&(Resistance>19051))
	{
		termo=10;
	}
	else if ((Resistance<=19051)&&(Resistance>18158))
	{
		termo=11;
	}
	else if ((Resistance<=18158)&&(Resistance>17312))
	{ //20-25C
		termo=12;
	}
	else if ((Resistance<=17312)&&(Resistance>16511))
	{ //25-30C
		termo=13;
	}
	else if ((Resistance<=16511)&&(Resistance>15751))
	{ //30-40C
		termo=14;
	}
	else if ((Resistance<=15751)&&(Resistance>15031))
	{ //40-50C
		termo=15;
	}
	else if ((Resistance<=15031)&&(Resistance>14347))
	{ //50-60C
		termo=16;
	}
	else if ((Resistance<=14347)&&(Resistance>13698))
	{ //60-70C
		termo=17;
	}
	else if ((Resistance<=13698)&&(Resistance>13083))
	{ //70-80C
		termo=18;
	}
	else if ((Resistance<=13083)&&(Resistance>12499))
	{ //80-90C
		termo=19;
	}
	else if ((Resistance<=12499)&&(Resistance>11944))
	{ //80-90C
		termo=20; //I don't care if it's above 91 either
	}
	else if ((Resistance<=11944)&&(Resistance>11417))
	{
		termo=21;
	}
	else if ((Resistance<=11417)&&(Resistance>10916))
	{ //20-25C
		termo=22;
	}
	else if ((Resistance<=10916)&&(Resistance>10440))
	{ //25-30C
		termo=23;
	}
	else if ((Resistance<=10440)&&(Resistance>10000))
	{ //30-40C
		termo=24;
	}
	else if ((Resistance<=10000)&&(Resistance>9556))
	{ //40-50C
		termo=25;
	}
	else if ((Resistance<=9556)&&(Resistance>9147))
	{ //50-60C
		termo=26;
	}
	else if ((Resistance<=9147)&&(Resistance>8387))
	{ //60-70C
		termo=27;
	}
	else if ((Resistance<=8758)&&(Resistance>8387))
	{ //70-80C
		termo=28;
	}
	else if ((Resistance<=8387)&&(Resistance>8034))
	{ //80-90C
		termo=29;
	}
	else if ((Resistance<=8034)&&(Resistance>7700))
	{ //80-90C
		termo=30; //I don't care if it's above 91 either
	}
	else if ((Resistance<=7700)&&(Resistance>7377))
	{
		termo=31;
	}
	else if ((Resistance<=7377)&&(Resistance>7072))
	{ //20-25C
		termo=32;
	}
	else if ((Resistance<=7072)&&(Resistance>6781))
	{ //25-30C
		termo=33;
	}
	else if ((Resistance<=6781)&&(Resistance>6504))
	{ //30-40C
		termo=34;
	}
	else if ((Resistance<=6504)&&(Resistance>6239))
	{ //40-50C
		termo=35;
	}
	else if ((Resistance<=6239)&&(Resistance>5987))
	{ //50-60C
		termo=36;
	}
	else if ((Resistance<=5987)&&(Resistance>5746))
	{ //60-70C
		termo=37;
	}
	else if ((Resistance<=5746)&&(Resistance>5517))
	{ //70-80C
		termo=38;
	}
	else if ((Resistance<=5517)&&(Resistance>5298))
	{ //80-90C
		termo=39;
	}
	else if (Resistance<5298)
	{ //80-90C
		termo=40; //I don't care if it's above 91 either
	}
	 return termo;
}

//
//void send_systemdata(int data)
//{
//	while (!(IFG2&UCA0TXIFG));  // Wait until TX buffer is ready
//	UCA0TXBUF = data;
//}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
	counter = counter + 1;
//	send_systemdata((int) (counter));
	if((counter)%2 == 0)
	{
		P1OUT ^= LED_0;
		send_data = 1;
		counter = 0;
	}
}


