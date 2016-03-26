/***************************************************************************************************
 *	        Include section					                       		   					       *
 ***************************************************************************************************/

#include <msp430.h>

/***************************************************************************************************
 *	        Prototype section					                       							   *
 ***************************************************************************************************/

unsigned int measure_adc(void);
unsigned int measuring_data(void);
void send_data(unsigned int measured_resistance);

/***************************************************************************************************
 *	        Global Variable section  				                            				   *
 ***************************************************************************************************/

unsigned int recieved_voltage;
unsigned int recieved_current;

unsigned int voltage;
unsigned int current;

unsigned int processed_current_data[10];
unsigned int processed_voltage_data;

unsigned int recieved_voltage;
unsigned int recieved_current;

unsigned int measured_voltage;
unsigned int measured_current;

unsigned int measured_resistance;
unsigned int calculated_resistance;

unsigned int dac_A_status = 12288;
unsigned int dac_B_status = 45056;
unsigned int sum;
unsigned int voltage_msb;
unsigned int voltage_lsb;
unsigned int current_msb;
unsigned int current_lsb;

unsigned int data_recieved;

char data_number = 0;


/***************************************************************************************************
 *	        Define section					                       		   					       *
 ***************************************************************************************************/


/***************************************************************************************************
 *         Main section                                                                            *
 ***************************************************************************************************/

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

	//Calibrate timer 1MHZ
	if(CALBC1_1MHZ==0xFF)
	{
		while(1);
	}
	DCOCTL = 0;
	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL = CALDCO_1MHZ;

    P1SEL  = BIT1 + BIT2 + BIT5 + BIT7;	 	//Enable module function on pin
    P1SEL2 = BIT1 + BIT2 + BIT5 + BIT7;  	//Select second module; P1.1 = RXD, P1.2=TXD

	P1DIR |= (BIT0 + BIT6);
	P1OUT &= ~ (BIT0 + BIT6);

	//For incoming UART
    UCA0CTL1 |= UCSSEL_2;    	// Set clock source SMCLK
    UCA0BR0 = 104;           	// 1MHz 9600 (N=BRCLK/baudrate)
    UCA0BR1 = 0;             	// 1MHz 9600
    UCA0MCTL = UCBRS0;       	// Modulation UCBRSx = 1
    UCA0CTL1 &= ~UCSWRST;    	// **Initialize USCI state machine**

    //For outgoing SPI
	UCB0CTL1   =   UCSWRST;
	UCB0CTL0   |=  UCCKPH  +   UCMSB   +   UCMST   +   UCSYNC; //  3-pin,  8-bit   SPI master
	UCB0CTL1   |=  UCSSEL_2;   //  SMCLK
	UCB0BR0    |=  0;   //  /2
	UCB0BR1    =   0;  //
	UCB0CTL1   &=  ~UCSWRST;   //  **Initialize    USCIstate   machine**

    IE2 |= UCA0RXIE;         	//enable RX interrupt

    __enable_interrupt(); 		//enable interrupt globally

	while(1){
		__bis_SR_register(GIE);
		__delay_cycles(1000);
		measured_resistance = measuring_data();
		__delay_cycles(1000);
		send_data(measured_resistance);
	}
}

void send_data(unsigned int measured_resistance){

	voltage = processed_voltage_data;
	current = processed_current_data[measured_resistance];

	sum = dac_A_status + voltage;

	voltage_msb = sum >> 8;
	voltage_lsb = sum;

	P1OUT  &= ~ (BIT0);    //  Select  Device
	UCB0TXBUF = voltage_msb;
	__delay_cycles(10);
	UCB0TXBUF = voltage_lsb;
	__delay_cycles(10);
	P1OUT  |=   (BIT0); //  Unselect    Device

	__delay_cycles(10);

	sum = dac_B_status + current;

	current_msb = sum >> 8;
	current_lsb = sum;

	P1OUT  &= ~ (BIT0);    //  Select  Device
	UCB0TXBUF = current_msb;
	__delay_cycles(10);
	UCB0TXBUF = current_lsb;
	__delay_cycles(10);
	P1OUT  |=   (BIT0); //  Unselect    Device

	__delay_cycles(10);
}

unsigned int measure_adc(void)
{
	unsigned int raw_value = 0;
	ADC10CTL0 |= ENC + ADC10SC;	// Sampling and conversion start
	while(ADC10CTL1 & BUSY); // Wait..i am converting...
	raw_value = ADC10MEM;	// Read ADC memory
	return raw_value;
}

unsigned int measuring_data(void)
{
	//Measure voltage
	ADC10CTL0 = ADC10SHT_3 + ADC10ON; //64 clocks for sample
	ADC10CTL1 = INCH_3 + ADC10DIV_3;	// Sensor is at channel X and CLK/4
	__delay_cycles(3000);	// settle time 30ms
	measured_voltage = measure_adc();
	measured_voltage = (measured_voltage*250)/1023;

	//Measure current
	ADC10CTL0 = ADC10SHT_3 + ADC10ON; //64 clocks for sample
	ADC10CTL1 = INCH_4 + ADC10DIV_3;	// Sensor is at channel X and CLK/4
	__delay_cycles(3000);	// settle time 30ms
	measured_current = measure_adc();
	measured_current = (measured_current*250)/1023;

	measured_resistance = measured_voltage / measured_current;
	return measured_resistance;
}

#pragma vector=USCIAB0RX_VECTOR		//interrupt service for recieving data
__interrupt void USCI0RX_ISR(void)
{
	data_recieved = UCA0RXBUF;

	if(data_number==0)
	{
		recieved_voltage = UCA0RXBUF;
		P1OUT ^= BIT6;
		__delay_cycles(1000);		//avoid bouncing
		P1OUT ^= BIT6;
		data_number = 1;
	}
	else
	{
		recieved_current = UCA0RXBUF;
		P1OUT ^= BIT6;
		__delay_cycles(1000);		//avoid bouncing
		P1OUT ^= BIT6;

		calculated_resistance = recieved_voltage/recieved_current;
		processed_current_data[calculated_resistance] = recieved_current;
		if((recieved_voltage != 0)&&(recieved_current == 0))
		{
			processed_voltage_data = recieved_voltage;
		}
		data_number = 0;
	}
}





