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
  0x95, 0x08,       // Report Count (8 fields)
  //0x95, 0x40,       // Report Count (8 fields)
  0x81, 0x02,       // Input (Data, Variable, Absolute)

  // Output report
  0x09, 0x03,       // Usage ID - vendor defined
  0x15, 0x00,       // Logical Minimum (0)
  0x26, 0xFF, 0x00, // Logical Maximum (255)
  0x75, 0x08,       // Report Size (8 bits)
  0x95, 0x08,       // Report Count (8 fields)
  //0x95, 0x40,       // Report Count (8 fields)
  0x91, 0x02,       // Output (Data, Variable, Absolute)
  
  //Feature report
  0x09, 0x04,       // Usage ID - vendor defined  
  0x15, 0x00,       // Logical Minimum (0)
  0x26, 0xFF, 0x00, // Logical Maximum (255)
  0x75, 0x08,       // Report Size (8 bits)
  0x95, 0x08,       // Report Count (8 fields)
  0xb2, 0x02, 0x01, //   FEATURE (Data,Var,Abs,Buf)
  

  0xc0              // END_COLLECTION
  };


///****Global variables****///
static uchar bytesRemaining;
static uchar currentAddress;
static uchar inbuf[8],local[8],frameRecived=0;
static uchar ReportID,passBy;








uchar   usbFunctionRead(uchar *data, uchar len)
{
    uchar j=0;
	if(len > bytesRemaining)
        len = bytesRemaining;
    
	for(j=0; j<len; j++)
        data[j] = local[j+currentAddress];
	
	data[1] = len;
    currentAddress += len;
    bytesRemaining -= len;
    return len;
}

uchar usbFunctionWrite (uchar *data, uchar len)
{
  uchar i;
  if (len > bytesRemaining) len = bytesRemaining;
	  bytesRemaining -= len;
  
  if(ReportID & SET_FEATURE){	  
    
	  frameRecived = 1;  
	  return bytesRemaining == 0;  // return 1 when done
	  setFeatureReport(SET_FEATURE,readAllowExtEeprom);
  }
  else{
	  makeNullArray(data,len,1);
	  return bytesRemaining == 0;  // return 1 when done
  }
}



usbMsgLen_t usbFunctionSetup (uchar *data)
{

  usbRequest_t    *rq = (void *)data;
  uchar incr =0;  

    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* HID class request */

        if(rq->bRequest == USBRQ_HID_GET_REPORT){  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
            /* since we have only one report type, we can ignore the report-ID */
            bytesRemaining = 0;			
            currentAddress = 0;
			
			ReportID=rq->wValue.bytes[1]; // ReportID stored in data[2]
			passBy = 1;
			return USB_NO_MSG;

            //return USB_NO_MSG;  /* use usbFunctionRead() to obtain data */
        }else if(rq->bRequest == USBRQ_HID_SET_REPORT){
			
			ReportID=rq->wValue.bytes[1]; // ReportID stored in data[2]
						
			if(ReportID == SET_FEATURE){
				
				bytesRemaining = 8;
				currentAddress = 0;			
						
				return USB_NO_MSG;			
			}
			else if(ReportID == SET_INPUT){
				bytesRemaining = 64;
				currentAddress = 0;			
						
				return USB_NO_MSG;	
			}
			
        }
    }else{
        /* ignore vendor type requests, we don't use any */
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

//unsigned char transferData[] = {1,1,1,1,1,3,3,1};



 int main(void)
 {
	 DDRB = 0b00000011;
     unsigned char ret;
	 timer1_init();
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
	 //ee24cxxx_write_bytes(0,sizeof(transferData),transferData)	 ;
	 
	 ee24cxxx_read_bytes(0,8,local);
	 
	 
	 
	 for(;;){
		wdt_reset();
		usbPoll();
		if(passBy){
			while(!usbInterruptIsReady()){
				wdt_reset();
				usbPoll();			
			}	
			 
			inbuf[0] = ReportID;
			usbSetInterrupt(inbuf,sizeof(inbuf));			
			passBy=0;
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





