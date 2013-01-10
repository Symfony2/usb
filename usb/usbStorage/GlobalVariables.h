/*
 * GlobalVariables.h
 *
 * Created: 14.08.2012 11:31:43
 *  Author: Алмаз
 */ 

/*
public enum AcknowledgeStatus
    {
        Good = 55,
        Bad  = 22,
        IdentificationFail = 12,
        None = 0

    }*/

#ifndef GLOBALVARIABLES_H_
#define GLOBALVARIABLES_H_

/***Общие величины***/



/***Типы операции***/
#define ERASE_EEPROM 10
#define READ_EEPROM 50
#define WRITE_TO_EEPROM 100
//#define READ_KEY 150
#define WRITE_KEY 200
#define ERASE_KEY 210
#define START_IDENTIFICATION 250

/***Типы ответов***/
#define GOOD 55
#define BAD 22
#define IDENTIFICATION_FAIL 12
#define IDENTIFICATION_PROCEED 18
#define EXTERNAL_EEPROM_IS_FULL 20
#define EXTERNAL_EEPROM_IS_EMPTY 23
#define UNABLE_TO_READ 25
#define UNABLE_TO_WRITE_KEY 27
#define KEY_STORAGE_IS_EMPTY 30


struct HostInteraction{
	unsigned char	
		isIdentificationProceed,			
		OperationType,
		DataSent,
		CommandNum,
		FrameRecived;
	
	unsigned int  TotalFrameAmount, CurrentFrame, Crc16;
	unsigned char buffer[56],Increment;
	
};

struct Timers{
/***Инкременты для таймера***/
	unsigned int inc_time0;
	unsigned int inc_time1;
	unsigned char enable_timer0;
	unsigned char enable_timer1;
};

volatile struct HostInteraction HostSlaveIntr;
/***Глобальные переменные таймера***/
volatile struct Timers time;

#endif /* GLOBALVARIABLES_H_ */