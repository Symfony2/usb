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

/***Типы операции***/
#define READ_EEPROM 50
#define WRITE_TO_EEPROM 100
#define READ_KEY 150
#define WRITE_KEY 200
#define START_IDENTIFICATION 250

/***Типы ответов***/
#define GOOD 55
#define BAD 55
#define IDENTIFICATION_FAIL 12


struct HostInteraction{
	unsigned char	
		isIdentificationProceed,			
		OperationType,
		DataSent,
		CommandNum,
		FrameRecived;
	
	unsigned int  Lenght, TotalFrameAmount, CurrentFrame;
	unsigned char buffer[64],Increment;
	
};

volatile struct HostInteraction HostSlaveIntr;

#endif /* GLOBALVARIABLES_H_ */