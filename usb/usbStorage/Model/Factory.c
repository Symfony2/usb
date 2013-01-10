/*
 * Factory.c
 *
 * Created: 20.08.2012 16:36:53
 *  Author: Алмаз
 */ 
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include "../usbdrv.h"
#include "../Model/Factory.h"
#include "../Model/IOmodel.h"



unsigned char checkCRC16(struct HostInteraction *fillStruct){
	unsigned char _buffer[DATA_ARRAY_LENGHT], inc = 0;
	
	makeNullArray(_buffer,DATA_ARRAY_LENGHT,0);
	
	for(inc=0;inc<DATA_ARRAY_LENGHT;inc++)
		_buffer[inc] = (*fillStruct).buffer[inc];
	
	return (*fillStruct).Crc16 == Crc16_clc(_buffer,DATA_ARRAY_LENGHT) ? 1 : 0;
}

unsigned char Factory(	unsigned char *recivedArray,
				unsigned char len,
				unsigned char *currentAddress,
				unsigned char *bytesRemaining,
				struct HostInteraction *fillStruct){	
				unsigned char j = 0;
		
	/***Проверка на количество байт***/
	if(*bytesRemaining == 0)		
        return 1;               

    /***Проверка на оставщиеся байты***/
	if(len > *bytesRemaining)
        len = *bytesRemaining;		 
	
	
	
	if(*currentAddress == 0){
		
	/***Здесь обрабатываеться шапка пакета***/
		(*fillStruct).CommandNum = recivedArray[0];
		(*fillStruct).OperationType = recivedArray[1];				
		(*fillStruct).Crc16 = recivedArray[2] | (recivedArray[3]<<8);
		(*fillStruct).TotalFrameAmount = recivedArray[4] | (recivedArray[5]<<8);		
		(*fillStruct).CurrentFrame = recivedArray[6] | (recivedArray[7]<<8);					
		
		*currentAddress += len;
		*bytesRemaining -= len;		
		return *bytesRemaining == 0;
	}
	else{
		
	/***Здесь обрабатываеться остальное тело пакета***/
		for(j=0; j < len; j++)
			(*fillStruct).buffer[j + (*fillStruct).Increment] = recivedArray[j];

		(*fillStruct).Increment+=len;
		*currentAddress += len;		
		*bytesRemaining -= len;		
			
		if(*bytesRemaining == 0){ 
						
			(*fillStruct).FrameRecived = 1;
			
			(*fillStruct).Increment = 0;
			Analyser(fillStruct);
		}
		return *bytesRemaining == 0; // recive data till the end
	}
}




void Analyser(struct HostInteraction *fillStruct){
	
	
	/***Проверка контрольной суммы пакета***/
	if(!checkCRC16(fillStruct)){
	/***Если произошла ошибка срс то ***/
		uAcknowledgeToHost((*fillStruct).CommandNum,BAD);
		return;
	}
	
	/***Запрос на идентификацию выполняеться внеочереди***/
	if((*fillStruct).OperationType == START_IDENTIFICATION){
		idenProc(fillStruct);
		return;
	}	
	
	/***Проверка идентификации осуществляеться проверкой флага по которому распознаеться время доверия истекло или нет..***/
	if(!(*fillStruct).isIdentificationProceed){
		uAcknowledgeToHost((*fillStruct).CommandNum, IDENTIFICATION_FAIL);
		return;	
	}		
				
	switch((*fillStruct).OperationType)
	{
		case READ_EEPROM:
			partialOutPutToHost(fillStruct);			
		break;
			
		case WRITE_TO_EEPROM:
			sequentialWriteToEeprom(fillStruct);
		break;
		
		case ERASE_EEPROM:
			eraseExternalEeprom(fillStruct);		
		break;		
					
		case WRITE_KEY:
			writeMD5(fillStruct);
		break;
		
		case START_IDENTIFICATION:
			idenProc(fillStruct);
		break;
		
		case ERASE_KEY:
			eraseMD5keys(fillStruct);			
		break;
			
		default:
		break;			
	}
				
	(*fillStruct).FrameRecived = 0;
}







