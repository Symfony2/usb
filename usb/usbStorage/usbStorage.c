/*
 * usbStorage.c
 *
 * Created: 21.06.2012 20:25:31
 *  Author: Алмаз
 */ 
#define F_CPU 12000000

#define UART_RX_BUFFER_SIZE 8
#define UART_TX_BUFFER_SIZE 8



#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include <avr/pgmspace.h>   /* нужно для usbdrv.h */

#include "usbdrv.h"
#include "uart.h"
#include "oddebug.h"        /* Этот также пример для использования макроса отладки */
#include "at24cxxx.h"
/* ------------------------------------------------------------------------- */
/* ----------------------------- интерфейс USB ----------------------------- */
/* ------------------------------------------------------------------------- */

PROGMEM char usbHidReportDescriptor[22] = {    /* дескриптор репорта USB */
    0x06, 0x00, 0xff,              // USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, 0x80,                    //   REPORT_COUNT (128)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)
    0xc0                           // END_COLLECTION
};

/* Следующие переменные сохраняют состояние текущей передачи данных */
static uchar    currentAddress;
static uchar    bytesRemaining;

uchar   usbFunctionRead(uchar *data, uchar len)
{
    if(len > bytesRemaining)
        len = bytesRemaining;
    eeprom_read_block(data, (uchar *)0 + currentAddress, len);
    currentAddress += len;
    bytesRemaining -= len;
    return len;
}

/* usbFunctionWrite() вызывается, когда хост посылает кусок данных в устройство.
 *  Для большей информации см. документацию в usbdrv/usbdrv.h.
 */
uchar   usbFunctionWrite(uchar *data, uchar len)
{
    if(bytesRemaining == 0)
        return 1;               /* окончание передачи */
    if(len > bytesRemaining)
        len = bytesRemaining;
    eeprom_write_block(data, (uchar *)0 + currentAddress, len);
    currentAddress += len;
    bytesRemaining -= len;
    return bytesRemaining == 0; /* возврат 1, если это был последний кусок */
}

/* ------------------------------------------------------------------------- */

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
usbRequest_t    *rq = (void *)data;

    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* запрос HID class */
        if(rq->bRequest == USBRQ_HID_GET_REPORT){  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
            /* поскольку мы имеем только один тип репорта, мы можем игнорировать репорт-ID */
            bytesRemaining = 128;
            currentAddress = 0;
            return USB_NO_MSG;  /* использование usbFunctionRead() для получения данных хостом от устройства */
        }else if(rq->bRequest == USBRQ_HID_SET_REPORT){
            /* поскольку мы имеем только один тип репорта, мы можем игнорировать репорт-ID */
            bytesRemaining = 128;
            currentAddress = 0;
            return USB_NO_MSG;  /* use usbFunctionWrite() для получения данных устройством от хоста */
        }
    }else{
        /* игнорируем запросы типа вендора, мы их все равно не используем */
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

unsigned char transferData[] = {3,3,3,5,5,4,4,4};


 int main(void)
 {
     unsigned char ret;
	 twi_init();
	 
	 ee24cxxx_write_bytes(0,sizeof(transferData),transferData)	 ;
	 for(;;);
 }
/*
unsigned char transferData[12] = {8,1,2,3,4,5,6,7,8};

int main(void)
{
	uart_init(UART_BAUD_SELECT(9600, F_CPU));
	DDRB = 0b00000010;	
	
	uart_puts(transferData);
	
	writePage64(transferData);
	
	sei();
    while(1)
    {
		
        //TODO:: Please write your application code 
    }
}*/





