/*
 * IOmodel.c
 *
 * Created: 22.07.2012 14:30:22
 *  Author: Алмаз
 */ 

/*General Host requests*/

#include "../Model/IOmodel.h"
#include <avr/io.h>
#include <avr/wdt.h>         // for wdt routines
#include <avr/interrupt.h>   // for sei()
#include <avr/pgmspace.h>
#include "../at24cxxx.h"
#include "../usbdrv.h"
#include "../oddebug.h"




// TIMERS -------> initialization

void timer1_init(void){
	
	TCCR1B = 0x05;
	TCNT1H = 0xD2;	
	TCNT1L = 0x38;
	TIMSK  = 0x04;
}



ISR(TIMER1_OVF_vect ){
	TCNT1H=0xD2;	
	TCNT1L=0x38;
	
	PORTB = PORTB & 0b00000001 ? 0b00000000 : 0b00000001;
}

char isIdentificationProceed = 1;

uint16_t _doMath(unsigned char hsb,unsigned char lsb, uint16_t _frameLenght){
            
    uint16_t  lenghOfdata = 0, operationValue = 0;

    lenghOfdata |= hsb << 8 | lsb;

    if (lenghOfdata < _frameLenght) lenghOfdata = _frameLenght;
    else
    {
        operationValue = ((lenghOfdata / _frameLenght) * _frameLenght);

        lenghOfdata = operationValue < lenghOfdata ?
            (operationValue + _frameLenght) : lenghOfdata;
    }
    return lenghOfdata;
}

void sequentialReadEeprom(void){
	
	unsigned char buffer[64];
	
	/*first two bytes of eeprom shows the lenght of data*/
	/*HSB and LSB*/
	unsigned char adressArray[2],incr=0;	
	uint16_t destinationAdress = 0;
	
	ee24cxxx_read_bytes(0,2,adressArray);
	
	destinationAdress = _doMath(adressArray[0],adressArray[1],FRAME_LENGHT);
	
	for(incr=0; incr < destinationAdress;incr += 64){
		ee24cxxx_read_bytes(incr,FRAME_LENGHT,buffer);
		while(!usbInterruptIsReady()){
			wdt_reset();
			usbPoll();
		}				 
						
		usbSetInterrupt(buffer,sizeof(buffer));
	}
	
}

void setFeatureReport(unsigned char reportType, unsigned char operationType){
	unsigned char i=0;	
	
	if(reportType & SET_FEATURE){
		if(isIdentificationProceed){
			switch(operationType){
				case readAllowExtEeprom:
					sequentialReadEeprom();
				break;
				case writeAllowExtEeprom:
					
				break;
				
			}
		}
		else{
			
		}
	}	
}

void makeNullArray(unsigned char* dataArray, unsigned char lenght, unsigned char defaultValue){
	unsigned char i = 0;
	for(i=0; i<lenght; i++)
		dataArray = defaultValue;
}










