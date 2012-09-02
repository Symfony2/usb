/*
 * Factory.h
 *
 * Created: 20.08.2012 17:13:17
 *  Author: Алмаз
 */ 


#ifndef FACTORY_H_
#define FACTORY_H_

#include "../GlobalVariables.h"

unsigned char Factory(	unsigned char *recivedArray,
				unsigned char len,
				unsigned char *currentAddress,
				unsigned char *bytesRemaining,
				struct HostInteraction *fillStruct);


#endif /* FACTORY_H_ */