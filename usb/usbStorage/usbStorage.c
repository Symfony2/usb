/*
 * usbStorage.c
 *
 * Created: 21.06.2012 20:25:31
 *  Author: Алмаз
 */ 
#define F_CPU 12000000L

#define UART_RX_BUFFER_SIZE 8
#define UART_TX_BUFFER_SIZE 8



#include <avr/io.h>
#include <avr/interrupt.h>
#include "at24cxxx.h"
#include "uart.h"



unsigned char transferData[7] = {1,1,1,1,1,1,1,1,1};


 int main(void)
 {
     unsigned char ret;
	 twi_init();
	 
	 ee24cxxx_write_bytes(0,sizeof(transferData),transferData)	 ;
	 for(;;);
 }
/*
unsigned char transferData[12] = {8,1,2,3,4,5,6,7,8};

int main(void)
{
	uart_init(UART_BAUD_SELECT(9600, F_CPU));
	DDRB = 0b00000010;	
	
	uart_puts(transferData);
	
	writePage64(transferData);
	
	sei();
    while(1)
    {
		
        //TODO:: Please write your application code 
    }
}*/





