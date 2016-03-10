#include    <msp430.h>

#define LED0 BIT0
#define LED1 BIT6

volatile    char    received_ch =   0;
int main(void)
{
	WDTCTL =   WDTPW   +   WDTHOLD;    //  Stop    WDT

//	P1DIR  |=  BIT4;
//	P1REN  |=  BIT4;
//	P1OUT  |=  BIT4;

	P1SEL  =   BIT5    |   BIT7;
	P1SEL2 =   BIT5    |   BIT7;

	P1DIR |= (LED0 + LED1);
	P1OUT &= ~ (LED0 + LED1);

	UCB0CTL1   =   UCSWRST;
	UCB0CTL0   |=  UCCKPH  +   UCMSB   +   UCMST   +   UCSYNC; //  3-pin,  8-bit   SPI master
	UCB0CTL1   |=  UCSSEL_2;   //  SMCLK
	UCB0BR0    |=  0;   //  /2
	UCB0BR1    =   0;  //
	UCB0CTL1   &=  ~UCSWRST;   //  **Initialize    USCI    state   machine**

	while(1)
	{
	P1OUT  |=  (LED0);    //  Select  Device

	while  (!(IFG2 &   UCB0TXIFG));    //  USCI_A0 TX  buffer  ready?
	UCB0TXBUF  =   0b10101010;   //  Send    0xAA    over    SPI to  Slave


//	while  (!(IFG2 &   UCA0RXIFG));    //  USCI_A0 RX  Received?
//	received_ch    =   UCA0RXBUF;  //  Store   received    data
//	P1OUT ^= LED1;
//	__delay_cycles(100000);
//	P1OUT ^= LED1;

	P1OUT  &= ~  (LED0); //  Unselect    Device

	}
}
