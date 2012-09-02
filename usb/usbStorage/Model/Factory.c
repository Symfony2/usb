/*
 * Factory.c
 *
 * Created: 20.08.2012 16:36:53
 *  Author: Алмаз
 */ 
#include <avr/io.h>
#include "../Model/Factory.h"
#include "../Model/IOmodel.h"

/*
unsigned char Factory(	unsigned char *recivedArray,
				unsigned char len,
				unsigned char *currentAddress,
				unsigned char *bytesRemaining,
				struct HostInteraction *fillStruct){	
unsigned char j = 0;
	/ ***Проверка идентификации*** /
	if(!(*fillStruct).isIdentificationProceed) 
	{
		*currentAddress=0;
		*bytesRemaining=0;
		return 1; //done
	}		
	/ ***Проверка на количество байт*** /
	if(*bytesRemaining == 0)		
        return 1;               

    / ***Проверка на оставщиеся байты*** /
	if(len > *bytesRemaining)
        len = *bytesRemaining;		
	
	(*fillStruct).Increment++;
	
	if(*currentAddress == 0){
		(*fillStruct).CommandNum = recivedArray[0] | (recivedArray[1]<<8);
		(*fillStruct).Lenght = recivedArray[4] | (recivedArray[5]<<8);
		(*fillStruct).OperationType = recivedArray[2] | (recivedArray[3]<<8);		
		*currentAddress += len;
		*bytesRemaining = (*fillStruct).Lenght;
		
		return *bytesRemaining!=0? 0 : 1; //
	}
	else{
		PORTB |= 2;
		for(j=0; j<len; j++)
			(*fillStruct).buffer[j+*currentAddress] = recivedArray[j];

		*currentAddress += len;
	    *bytesRemaining -= len;		
			
		if(*bytesRemaining == 0){ 
			(*fillStruct).DataRecived = 1;
			(*fillStruct).Increment = 0;
		}
		return *bytesRemaining == 0; // recive data till the end
	}		
}*/

unsigned char Factory(	unsigned char *recivedArray,
				unsigned char len,
				unsigned char *currentAddress,
				unsigned char *bytesRemaining,
				struct HostInteraction *fillStruct){	
unsigned char j = 0;
	/***Проверка идентификации***/
	if(!(*fillStruct).isIdentificationProceed) 
	{
		*currentAddress=0;
		*bytesRemaining=0;
		return 1; //done
	}		
	/***Проверка на количество байт***/
	if(*bytesRemaining == 0)		
        return 1;               

    /***Проверка на оставщиеся байты***/
	if(len > *bytesRemaining)
        len = *bytesRemaining;		
	
	
	
	if(*currentAddress == 0){
		(*fillStruct).CommandNum = recivedArray[0];
		(*fillStruct).OperationType = recivedArray[1];				
		(*fillStruct).Lenght = recivedArray[2] | (recivedArray[3]<<8);
		(*fillStruct).TotalFrameAmount = recivedArray[4] | (recivedArray[5]<<8);		
		(*fillStruct).CurrentFrame = recivedArray[6] | (recivedArray[7]<<8);	
		return 0;
	}
	else{
		PORTB |= 2;	
	
		for(j=0; j < len; j++)
			(*fillStruct).buffer[j+*currentAddress] = recivedArray[j];

		*currentAddress += len;
		*bytesRemaining -= len;		
			
		if(*bytesRemaining == 0){ 
			(*fillStruct).Increment++;
			(*fillStruct).FrameRecived = 1;
			(*fillStruct).Increment = 0;
		}
		return *bytesRemaining == 0; // recive data till the end
	}
			
}




void Analyser(struct HostInteraction *fillStruct){	
		
	(*fillStruct).FrameRecived = 0;
		
	switch((*fillStruct).OperationType)
	{
		case READ_EEPROM:
				
		break;
			
		case WRITE_TO_EEPROM:
			sequentialWriteToEeprom(**fillStruct,56);
		break;
			
		case READ_KEY:			
		break;
			
		case WRITE_KEY:			
		break;
			
		default:
		break;			
	}			
	
}







