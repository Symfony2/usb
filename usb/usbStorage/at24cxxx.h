/*
at24cxxx.h:

for at24c128/256, have been tested on at24c256 (32Kx8bit, PageSize=64bytes),
and atmega48(8MHzRC).

test hardware:
at24c256 <--> atmega48
A0-->GND
A1-->GND
GND-->GND
SDA----->SDA(PC4 of mega48)
     |
    1KOhm
     |
     V
    VCC

SCL----->SCL(PC5 of mega48)
     |
    1KOhm
     |
     V
    VCC

WP-->GND
VCC-->VCC
(VCC-->0.1uF-->GND)

references:
1)at24c128/256 datasheet.
2)atmega48 datasheet.
3)demo project of avr-libc-manual-1.4.0.
*/

#include <inttypes.h>
#include <stdlib.h>

#include <avr/io.h>
#include <util/twi.h>
/* for future enhancement*/
#include <avr/interrupt.h>

/* twi address for at24c128/256:
1 0 1 0 0 A1 A0 R/W
*/
#define SLA_W_24CXXX 0xA0
#define SLA_R_24CXXX 0xA1

/* use mega48 interal RC 8MHz/1=8MHz lfuse=0xe2*/

/* page size is 64 byte, and address word is 15 bit (at24c256)
   =high 7 bit(as first address word)+low 8 bit(as second address word). */
#define PAGE_SIZE 64

/* set prescaler to 1.(TWPS0,1=0)*/
#define TWI_PS 0

/* save value of TW_STATUS */
uint8_t twst;

void twi_init(void);
int ee24cxxx_write_page(uint16_t addr, int len, uint8_t *buf);
int ee24cxxx_write_bytes(uint16_t addr, int len, uint8_t *buf);
int ee24cxxx_read_bytes(uint16_t addr, int len, uint8_t *buf);