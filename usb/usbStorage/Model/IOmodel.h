/*
 * IOmodel.h
 *
 * Created: 23.07.2012 16:46:15
 *  Author: Алмаз
 */ 



#ifndef IOMODEL_H_
#define IOMODEL_H_

/*Global constants*/
#define readAllowExtEeprom    0x0a
#define writeAllowExtEeprom   0x0b
#define readKey               0x0c
#define writeKey              0x0d

/*Report types*/
#define SET_FEATURE 3
#define GET_FEATURE 3

#define SET_INPUT  2
#define GET_OUTPUT 1

#define EXTERNAL_EEPROM_SIZE 0x8000
#define FRAME_LENGHT 64


void timer1_init(void);

ISR(TIMER1_OVF_vect );

void makeNullArray(unsigned char* dataArray, unsigned char lenght, unsigned char defaultValue);

void sequentialReadEeprom(void);


#endif /* IOMODEL_H_ */