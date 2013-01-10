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
#include <avr/eeprom.h>
#include "../at24cxxx.h"
#include "../usbdrv.h"
#include "../oddebug.h"






// TIMERS -------> initialization

void timers_init(void){
	// Timer/Counter 0 initialization
	// Clock source: System Clock
	// Clock value: 46,875 kHz
	TCCR0=0x04;
	TCNT0=0x00;
	// Timer/Counter 1 initialization
	// Clock source: System Clock
	// Clock value: 11,719 kHz

	TCCR1B=0x05;
	TCNT1H=0x00;
	TCNT1L=0x00;
		
}



ISR(TIMER0_OVF_vect ){	
	
	if(!time.enable_timer0)
		return;
	if(time.inc_time0++ == 50){
		time.inc_time0 = 0;
		PORTB = PORTB & 0b00000001 ? 0b00000000 : 0b00000001;
	}
}

ISR(TIMER1_OVF_vect ){	
	
	if(!time.enable_timer1)
		return;
	time.enable_timer0 = 1;
	if(time.inc_time1++ == 25){
		time.inc_time1 = 0;
/***При прошествии 2.33 минут останавливаем работу таймера 1 и таймера 0***/
		time.enable_timer0 = 0;
		time.enable_timer1 = 0;
		 PORTB = 0;
		HostSlaveIntr.isIdentificationProceed = 0;	
	}
}
	

void makeNullArray(unsigned char* dataArray, unsigned int lenght, unsigned char defaultValue){
	unsigned int i = 0;
	for(i=0; i<lenght; i++){
		wdt_reset();
		usbPoll();	
		dataArray[i] = defaultValue;
	}
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


unsigned int Crc16_clc( unsigned char *Data, unsigned int DataLen){

	unsigned int Temp;
	unsigned int Crc;

	Crc = 0; 

	while (DataLen--) {
		wdt_reset();
		usbPoll();
		Temp = (unsigned int)(*Data++) ^ (Crc >> 8);
		Temp ^= (Temp >> 4);
		Temp ^= (Temp >> 2);
		Temp ^= (Temp >> 1);
		Crc = (Crc <<8) ^ (Temp << 15) ^ (Temp << 2) ^ Temp;
	} 

return(Crc);
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

/*Sends protocoled frame to host with size of 64 byte*/
void sendProtocolFrameToHost(	unsigned char	_commandNumber, 
								unsigned char	_ackType, 
								unsigned int	_calculatedCRC16,
								unsigned int	_totalFrameAmount,
								unsigned int	_currentFrameNumber,
								unsigned char	* dataArray){
	
	/*Lenght of data array consists of 56 bytes*/
	unsigned char	buffer[8] = 
						{_commandNumber, 
						_ackType, 
						_calculatedCRC16, 
						_calculatedCRC16>>8, 
						_totalFrameAmount, 
						_totalFrameAmount>>8, 
						_currentFrameNumber, 
						_currentFrameNumber>>8}
						, inc = 0, i = 0;
						
	unsigned char resultArray[64];
	
	for(inc = 0; inc < 64; inc++)
		resultArray[inc] = inc < 8 ? buffer[inc] : dataArray[inc - 8];	
	
	makeNullArray(buffer,8,0);
	for(inc=0; inc < 64; inc += 8){
		for(i=0; i<8; i++)
			buffer[i] = resultArray[inc+i];
		while(!usbInterruptIsReady()){
				wdt_reset();
				usbPoll();
			}						
		usbSetInterrupt(buffer,8);	
	}
}



void consequentiallyOutPutFramesToHost(struct HostInteraction *dataStructure){
	
	unsigned char adressArray[2], _buffer[DATA_ARRAY_LENGHT];	
	uint16_t	dataFramesAmount = 0,inc = 0,
				dataLenght, calcCRC16 = 0;
	
	if(!(*dataStructure).FrameRecived)
		return;

	wdt_reset();
	usbPoll();
	ee24cxxx_read_bytes(0,2,adressArray);
	dataLenght = (adressArray[1]<<8)| adressArray[0];
	
		
	/***Проверка на верхний край***/
	if(dataLenght > EXTERNAL_EEPROM_SIZE)
		return;	
		
	/***Проверка на нижний край***/
	if(dataLenght == 0){
		uAcknowledgeToHost((*dataStructure).CommandNum,EXTERNAL_EEPROM_IS_EMPTY);
		return;
	}
		
		
	dataFramesAmount = framesAmount(dataLenght, DATA_ARRAY_LENGHT);
	/***Поочередно отправляем данные хосту***/
	for(inc = 0; inc < dataFramesAmount; inc++){
		makeNullArray(_buffer,DATA_ARRAY_LENGHT,0);
		ee24cxxx_read_bytes(inc * DATA_ARRAY_LENGHT, DATA_ARRAY_LENGHT, _buffer);
		calcCRC16 = Crc16_clc(_buffer, DATA_ARRAY_LENGHT);		
		
		sendProtocolFrameToHost((*dataStructure).CommandNum, GOOD,calcCRC16,dataFramesAmount, inc + 1 , _buffer);
	}	
}
/***Метод выполняет чтение с внешнего eeprom вычисляет СРС и строго по кадрам передает хосту запрошенный кусок
каждый раз читаем адресную часть и формируем общую картину. Узнаем общую длину файла вычисляем общее количество кадров***/
void partialOutPutToHost(struct HostInteraction *dataStructure){
	
	unsigned char adressArray[2], _buffer[DATA_ARRAY_LENGHT];	
	uint16_t	dataFramesAmount = 0, readFromAdrr = 0, inc = 0,
				dataLenght, calcCRC16 = 0;
	
	ee24cxxx_read_bytes(0,2,adressArray);
	dataLenght = (adressArray[1]<<8)| adressArray[0];
	
	/***Проверка на верхний край***/
	if(dataLenght > EXTERNAL_EEPROM_SIZE)
		return;	
		
	/***Проверка на нижний край***/
	if(dataLenght == 0){
		uAcknowledgeToHost((*dataStructure).CommandNum,EXTERNAL_EEPROM_IS_EMPTY);
		return;
	}
	
	readFromAdrr = ((*dataStructure).CurrentFrame - 1)*DATA_ARRAY_LENGHT;	
	
	if((EXTERNAL_EEPROM_SIZE-readFromAdrr) < DATA_ARRAY_LENGHT )
		uAcknowledgeToHost((*dataStructure).CommandNum, UNABLE_TO_READ);
	
	//Здесь мы прочитали запрошенный кусок
	ee24cxxx_read_bytes(readFromAdrr, DATA_ARRAY_LENGHT,_buffer);
	
	calcCRC16 = Crc16_clc(_buffer, DATA_ARRAY_LENGHT);		
		
	sendProtocolFrameToHost((*dataStructure).CommandNum, GOOD,calcCRC16,(*dataStructure).TotalFrameAmount, inc + 1 , _buffer);
		
}
/***Выполняет простую запись 56 байт в память***/
void sequentialWriteToEeprom(struct HostInteraction *dataStructure){
	unsigned int min = 0; 
			
	
	min = ((*dataStructure).CurrentFrame - 1) * DATA_ARRAY_LENGHT;
	
	/***Проверяем подошли ли мы к краю? если да то не выполняем следующее действие
	и отправляем хосту сигнал о том что память полна***/
	if((min + DATA_ARRAY_LENGHT)>EXTERNAL_EEPROM_SIZE){
		uAcknowledgeToHost((*dataStructure).CommandNum,EXTERNAL_EEPROM_IS_FULL);
		return;
	}		
	
	wdt_reset();
	usbPoll();
	ee24cxxx_write_bytes(min,DATA_ARRAY_LENGHT,(*dataStructure).buffer);
	
	
	/***После записи отвечаем хосту что у нас все ОК***/	
	uAcknowledgeToHost((*dataStructure).CommandNum,GOOD);
}


/***По кадровое стирание памяти***/
void eraseExternalEeprom(struct HostInteraction *dataStructure){
	
	unsigned char emptyBuffer[PAGE_SIZE];
	/***Выключаем таймер ограничивающее время Потому что стираються и все ключи***/
	time.enable_timer1 = 0;
	
	/***Page size is 64***/	
	makeNullArray(emptyBuffer,PAGE_SIZE,0xFF);
	emptyBuffer[0] = emptyBuffer[1] = (*dataStructure).CurrentFrame == 1 ? 0 : 0xFF;
	
	ee24cxxx_write_page(((*dataStructure).CurrentFrame-1) * PAGE_SIZE, PAGE_SIZE, emptyBuffer);
	eraseMD5keys(dataStructure);
	/***Команда выполненно успешно***/
	uAcknowledgeToHost((*dataStructure).CommandNum, GOOD);
}



/*
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

/ ***Служебный метод для вывода массива определенной длины*** /
void serviceOutPut(unsigned int *array, unsigned int lenght){
	unsigned char	packet[64];
	unsigned int	frameAmount = 0, 				
					border = 0,
					y = 0, 
					x = 0;
	
	
	frameAmount = framesAmount(lenght, 64);	
	
	for(x=0; x<frameAmount; x++){
		
		makeNullArray(packet,64, 0);				
				
		border = (x+1) == frameAmount ? lenght - x*64 : 64;
		
		for( y = 0; y < border; y++ )
				packet[y] = array[x*64 + y];
				
												
		print64bytes(packet);			
		
	}	
}*/

/***ПРОЦЕСС ИДЕНТИФИКАЦИИ***/

void writeMD5(struct HostInteraction *dataStructure){
		
	unsigned char keysAmount, md5[16], inc = 0;
	keysAmount = eeprom_read_byte(0);
	
	/***keysAmount не может быть больше 2 ключей!!!!***/
	if(keysAmount>1){
		uAcknowledgeToHost((*dataStructure).CommandNum,UNABLE_TO_WRITE_KEY);
		return;
	}
	eeprom_write_byte(0, ++keysAmount);
	for(inc = 0; inc<16; inc++)
		md5[inc] = (*dataStructure).buffer[inc];
	keysAmount = (keysAmount-1)*16 + 1; //избегаем 1 байт так как он отвечает за адрес
	eeprom_write_block(md5, keysAmount,16);
	
	/***После записи отвечаем хосту что у нас все ОК***/	
	uAcknowledgeToHost((*dataStructure).CommandNum,GOOD);		
}
/***Метод производящий идентификацию***/
void idenProc(struct HostInteraction *dataStructure){
	/***keysAmount не может быть больше 2 ключей!!!!***/
	unsigned char	keysAmount, 
					md5[16], 
					inc = 0,
					i,j,adress;
	keysAmount = eeprom_read_byte(0);
	/***Если в памяти нет ключей сообщяем хосту***/
	if(keysAmount == 0){
		uAcknowledgeToHost((*dataStructure).CommandNum,KEY_STORAGE_IS_EMPTY);
		return;
	}
	for(i=0;i<keysAmount;i++){
		adress = i * 16 + 1; ///пропускаем 1 байт так как он отвечает за количество ключей в памяти
		eeprom_read_block(md5, adress, 16);
		for(j=0; j<16; j++){
			if((*dataStructure).buffer[j] == md5[j])
				inc++;									
		}
		if(inc == 16){
			uAcknowledgeToHost((*dataStructure).CommandNum,IDENTIFICATION_PROCEED);
			(*dataStructure).isIdentificationProceed = 1;
			time.enable_timer1 = 1;
			return;
		}
		inc = 0;
	}
	uAcknowledgeToHost((*dataStructure).CommandNum, IDENTIFICATION_FAIL);
}

/***Метод стирающий область ключей***/
void eraseMD5keys(struct HostInteraction *dataStructure){
/***Очищяем 33 байта в памяти это 2 ключа по 16 байт и 1 байт адресса***/
	unsigned char inc = 0;
	for(inc = 0; inc<33; inc++)
		eeprom_write_byte(inc,0);
		
	uAcknowledgeToHost((*dataStructure).CommandNum, GOOD);		
	(*dataStructure).isIdentificationProceed = 1;
}


/***ОТВЕТЫ ДЛЯ ХОСТА***/


void uAcknowledgeToHost(unsigned char commandNumber,unsigned char commType){
	unsigned char	i = 0, j = 0,
					dataArray[DATA_ARRAY_LENGHT];
	makeNullArray(dataArray,sizeof(dataArray),0);	
	sendProtocolFrameToHost(commandNumber,commType,0,1,1,dataArray);
}












