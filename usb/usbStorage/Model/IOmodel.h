/*
 * IOmodel.h
 *
 * Created: 23.07.2012 16:46:15
 *  Author: Алмаз
 */ 



#ifndef IOMODEL_H_
#define IOMODEL_H_

#include "../GlobalVariables.h"

/*Global constants*/
#define readAllowExtEeprom    0x0a
#define writeAllowExtEeprom   0x0b
#define readKey               0x0c
#define writeKey              0x0d

/*Report types*/
#define DATA_ARRAY_LENGHT 56


#define EXTERNAL_EEPROM_SIZE 0x8000
#define FRAME_LENGHT 8


void timers_init(void);

//ISR(TIMER1_OVF_vect );

void makeNullArray(unsigned char* dataArray, unsigned int lenght, unsigned char defaultValue);

unsigned int Crc16_clc( unsigned char *Data, unsigned int DataLen);

void sequentialWriteToEeprom(struct HostInteraction *dataStructure);

void consequentiallyOutPutFramesToHost(struct HostInteraction *dataStructure);

void partialOutPutToHost(struct HostInteraction *dataStructure);

//void serviceOutPut(unsigned int *array, unsigned int lenght);

void eraseExternalEeprom(struct HostInteraction *dataStructure);

void idenProc(struct HostInteraction *dataStructure);

void writeMD5(struct HostInteraction *dataStructure);

void uAcknowledgeToHost(unsigned char commandNumber,unsigned char commType);

void eraseMD5keys(struct HostInteraction *dataStructure);

//void print64bytes(unsigned char *array);


#endif /* IOMODEL_H_ */