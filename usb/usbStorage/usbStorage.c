/*
 * usbStorage.c
 *
 * Created: 21.06.2012 20:25:31
 *  Author: Алмаз
 */ 
#define F_CPU 12000000

/*
#define UART_RX_BUFFER_SIZE 8
#define UART_TX_BUFFER_SIZE 8*/



#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>         // for wdt routines

#include <avr/pgmspace.h>   /* нужно для usbdrv.h */

#include "GlobalVariables.h"

#include "usbdrv.h"
#include "uart.h"
#include "oddebug.h"        /* Этот также пример для использования макроса отладки */
#include "at24cxxx.h"
#include "Model/IOmodel.h"

#include <util/delay.h>      // for _delay_ms()
#include <avr/eeprom.h>      // memory

volatile struct HostOrder{
	unsigned char hostRequest;	
}; 


struct HostOrder hostOrder = {0};

/* ------------------------------------------------------------------------- */
/* ----------------------------- интерфейс USB ----------------------------- */
/* ------------------------------------------------------------------------- */

/* USB report descriptor */
PROGMEM char usbHidReportDescriptor[] = {
  0x06, 0xa0, 0xff, // USAGE_PAGE (Vendor Defined Page 1)
  0x09, 0x01,       // USAGE (Vendor Usage 1)
  0xa1, 0x01,       // COLLECTION (Application)

  // Input Report
  0x09, 0x02,       // Usage ID - vendor defined
  0x15, 0x00,       // Logical Minimum (0)
  0x26, 0xFF, 0x00, // Logical Maximum (255)
  0x75, 0x08,       // Report Size (8 bits)
  //0x95, 0x08,       // Report Count (8 fields)
  0x95, 0x40,       // Report Count (8 fields)
  0x81, 0x02,       // Input (Data, Variable, Absolute)

  // Output report
  0x09, 0x03,       // Usage ID - vendor defined
  0x15, 0x00,       // Logical Minimum (0)
  0x26, 0xFF, 0x00, // Logical Maximum (255)
  0x75, 0x08,       // Report Size (8 bits)
  //0x95, 0x08,       // Report Count (8 fields)
  0x95, 0x40,       // Report Count (8 fields)
  0x91, 0x02,       // Output (Data, Variable, Absolute)
  
  /*//Feature report
  0x09, 0x04,       // Usage ID - vendor defined  
  0x15, 0x00,       // Logical Minimum (0)
  0x26, 0xFF, 0x00, // Logical Maximum (255)
  0x75, 0x08,       // Report Size (8 bits)
  0x95, 0x08,       // Report Count (8 fields)
  0xb2, 0x02, 0x01, //   FEATURE (Data,Var,Abs,Buf)  */

  0xc0              // END_COLLECTION
  };


///****Global variables****///
static uchar bytesRemaining;
static uchar currentAddress;

static uchar ReportID,passBy,checkPoint=0, inbuff[8];








uchar   usbFunctionRead(uchar *data, uchar len)
{
    uchar j=0;
	if(len > bytesRemaining)
        len = bytesRemaining;
   
    if(HostSlaveIntr.OperationType == readAllowExtEeprom){
	   
	for(j=0; j<len; j++)
        data[j] = HostSlaveIntr.buffer[j+currentAddress];
	}
	else{
		makeNullArray(data,8,0);		
	}				
		
    currentAddress += len;
    bytesRemaining -= len;
	
	if(bytesRemaining == 0){ 		
		HostSlaveIntr.DataSent = 1;
		HostSlaveIntr.OperationType = 0;
	}		
	else
		HostSlaveIntr.DataSent = 0;
		
    return len;
}

uchar usbFunctionWrite (uchar *data, uchar len)
{
	uchar i;
	if (len > bytesRemaining) len = bytesRemaining;
	
	for(i=0; i<len; i++)
        HostSlaveIntr.buffer[i+currentAddress] = data[i];
	  
	currentAddress += len;
	bytesRemaining -= len;    
	
	
	  
	if(bytesRemaining == 0){
		HostSlaveIntr.DataRecived = 1;
		HostSlaveIntr.OperationType = 0;
		passBy=1;
	}
	else
		HostSlaveIntr.DataRecived = 0;
	
	
	return bytesRemaining == 0;  // return 1 when done	
	//return Factory(data, len, &currentAddress, &bytesRemaining, &HostSlaveIntr);
}



usbMsgLen_t usbFunctionSetup (uchar *data)
{

  usbRequest_t    *rq = (void *)data;
  uchar incr =0;  
  

    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* HID class request */

        if(rq->bRequest == USBRQ_HID_GET_REPORT){  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
            /* since we have only one report type, we can ignore the report-ID */
            
			
			ReportID=rq->wValue.bytes[1]; // ReportID stored in data[2]
			
			bytesRemaining = 64;			
			currentAddress = 0;			
		
			return USB_NO_MSG;			
			
            //return USB_NO_MSG;  /* use usbFunctionRead() to obtain data */
        }else if(rq->bRequest == USBRQ_HID_SET_REPORT){
			
			ReportID=rq->wValue.bytes[1]; // ReportID stored in data[2]
						
			bytesRemaining = 64;
			currentAddress = 0;
						
			return USB_NO_MSG;
			
        }
    }else{
        /* ignore vendor type requests, we don't use any */
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
//unsigned char transferData[] = {4,4,4,4,4,3,3,4};



 int main(void)
 {
	 
	 DDRB = 0b00000011;
     
	 timer1_init();
	 TIMSK  = 0x00;          /*timers interrupt enable*/
	 //TIMSK  = 0x04;
	 twi_init();
	 wdt_enable(WDTO_2S);

    usbInit();
	
    usbDeviceDisconnect();  // принудительно отключаемся от хоста, так делать можно только при выключенных прерываниях!
    
    uchar i = 0;
    while(--i){             // пауза > 250 ms
		wdt_reset();
        _delay_ms(1);
    }
    
    usbDeviceConnect();   
	 sei();
	 //ee24cxxx_write_bytes(16,sizeof(transferData),transferData);	 
	 //ee24cxxx_read_bytes(0,64,local); 
	 HostSlaveIntr.isIdentificationProceed = 1;
	 for(;;){
		wdt_reset();
		usbPoll();
		
		/*if(HostSlaveIntr.DataRecived){
			
			HostSlaveIntr.DataRecived=0;
			HostSlaveIntr.OperationType = readAllowExtEeprom;			
			serviceOutPut(HostSlaveIntr.buffer);			
		}*/
		
		if(passBy){
			passBy=0;
			inbuff[0] = checkPoint;
			serviceOutPut(inbuff);
			checkPoint = 0;
		}
		
				
	 }		
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





