/*
 * I2cMemory.c
 *
 * Created: 21.06.2012 20:28:33
 *  Author: Алмаз
 */ 
#include "../Memory/I2cMemory.h"
#include "../i2cmaster/i2cmaster.h"
#define Dev24C02  0xA0




void writePage64(unsigned char *dataArray){
	
	/*unsigned int i=0;
	unsigned char  ret = 0;
	unsigned int increment = 0;	
	unsigned int arrayLenght = sizeof(dataArray);
	char iterationAmount = arrayLenght/64;
	
	metka:
	i2c_init();		
	for(i=0;i<iterationAmount;i++){
		ret = i2c_start(Dev24C02+I2C_WRITE);
	
		if(ret!=0){
			i2c_write((i*64)>>8);   //HSB
			i2c_write(i*64);        //LSB
			
			while(increment++<64){
				i2c_write(dataArray[increment]);
			}
			i2c_stop();
		}
		else{
			goto metka;
		}		*/
		i2c_init();                             // initialize I2C library

		// write 0x75 to EEPROM address 5 (Byte Write) 
		i2c_start_wait(Dev24C02+I2C_WRITE);     // set device address and write mode
		
		i2c_write(0x05);                        // write address = 5
		i2c_write(0x75);                        // write value 0x75 to EEPROM
		i2c_stop(); 
	
} 