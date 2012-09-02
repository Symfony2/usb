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
	
}



ISR(TIMER1_OVF_vect ){
	TCNT1H=0xD2;	
	TCNT1L=0x38;
	
	PORTB = PORTB & 0b00000001 ? 0b00000000 : 0b00000001;
}

void makeNullArray(unsigned char* dataArray, unsigned int lenght, unsigned char defaultValue){
	unsigned int i = 0;
	for(i=0; i<lenght; i++)	
		dataArray[i] = defaultValue;
}

 uint16_t framesAmount(uint16_t lenghOfdata, uint16_t _frameLenght)
{
    uint16_t operationValue = 0;
            
    if (lenghOfdata < _frameLenght) lenghOfdata = _frameLenght;
    else
    {
		    
        operationValue = ((lenghOfdata / _frameLenght) * _frameLenght);
		
        lenghOfdata = operationValue < lenghOfdata ?
            (operationValue + _frameLenght) : lenghOfdata;
    }
    return lenghOfdata/_frameLenght;
}

uint16_t _doMath(unsigned char hsb,unsigned char lsb, uint16_t _frameLenght){
            
    uint16_t  lenghOfdata = 0, operationValue = 0;

    lenghOfdata |= hsb << 8 | lsb;

    if (lenghOfdata < _frameLenght) lenghOfdata = _frameLenght;
    else
    {
		wdt_reset();
		usbPoll();
        operationValue = ((lenghOfdata / _frameLenght) * _frameLenght);
		
        lenghOfdata = operationValue < lenghOfdata ?
            (operationValue + _frameLenght) : lenghOfdata;
    }
    return lenghOfdata;
}

void sequentialReadEeprom(void){
	
	unsigned char buffer[8];		
	/*first two bytes of eeprom shows the lenght of data*/
	/*HSB and LSB*/	
	unsigned char adressArray[2],j=0;	
	uint16_t destinationAdress = 0,incr=0;
	wdt_reset();
	usbPoll();
	ee24cxxx_read_bytes(0,2,adressArray);
	
	destinationAdress = _doMath(adressArray[1],adressArray[0],FRAME_LENGHT);
	
	/*
	makeNullArray(buffer,8,0);
	buffer[0] = destinationAdress>>8; //hsb
	buffer[1] = destinationAdress;		//lsb
	
	for(incr=0; incr < 64; incr += 8){
		
		wdt_reset();
		usbPoll();		
		while(!usbInterruptIsReady()){
				wdt_reset();
				usbPoll();
			}				 
						
		usbSetInterrupt(buffer,8);	
	}	
	*/
	
	for(incr=0; incr < destinationAdress; incr += 8){
		wdt_reset();
		usbPoll();
		
		ee24cxxx_read_bytes(incr,FRAME_LENGHT,buffer);
		
		while(!usbInterruptIsReady()){
				wdt_reset();
				usbPoll();
			}						
		usbSetInterrupt(buffer,8);	
	}	
}
/***Выполняет простую запись 56 байт в память***/
void sequentialWriteToEeprom(struct HostInteraction *dataStructure, unsigned char dataLenghtPerOneFrame){
	unsigned int min; 
	unsigned char address[2];
	
	/***Принимаем первый фрейм производим запись 0(lsb) и 1(hsb) адреса 
		в памяти которые отвечают за длину всего файла записанного***/
	address[1] = (*dataStructure).Lenght>>8;
	address[0] = (*dataStructure).Lenght;
	wdt_reset();
	usbPoll();
	ee24cxxx_write_bytes(0,2,address);
	
	
	min = ((*dataStructure).CurrentFrame - 1) * dataLenghtPerOneFrame + 2;
	
	wdt_reset();
	usbPoll();
	ee24cxxx_write_bytes(min,dataLenghtPerOneFrame,(*dataStructure).buffer);
}

void print64bytes(unsigned char *array){
	unsigned char i=0, j=0, buff[8];
	for(i=0; i<64; i+=8){
		for(j=0; j<8; j++)
			buff[j]=array[i+j];
			
		while(!usbInterruptIsReady()){
				wdt_reset();
				usbPoll();
			}						
		usbSetInterrupt(buff,8);
	}
}

/***Служебный метод для вывода массива определенной длины***/
void serviceOutPut(unsigned int *array){
	unsigned char  packet[64],inc=0;
	unsigned int lenght = 0, 
				frameAmount = 0, 				
				border = 0,
				i = 0, 
				j = 0;
	
	lenght = sizeof(array);
	frameAmount = framesAmount(lenght, 64);	
	
	for(j=0; j<frameAmount; j++){
		
		makeNullArray(packet,64, 0);		
		inc++;
		
		border = inc == frameAmount ? lenght - (inc-1)*64 : 64;
		
		for(i=0;i<border;i++)
				packet[i] = array[j*64 + i];
												
		print64bytes(packet);			
		
	}	
}

/***ОТВЕТЫ ДЛЯ ХОСТА***/

void prefixData(unsigned char ackStatus)
{
	unsigned char dataArray[8], inc=0;
	
		
}	
 
void sendExtEepromDataToHost()
{
		
}












