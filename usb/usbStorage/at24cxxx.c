/*
at24cxxx.c:

definition of functions.
see at24cxxx.h for details.
*/

#include "at24cxxx.h"

void twi_init(void)
{
  /* set prescaler*/
  TWSR = TWI_PS;

#if F_CPU < 3600000UL
  TWBR = 10; /* smallest TWBR value. */
#else
  TWBR = (F_CPU / 100000UL - 16) / 2;
#endif
}

/* page write, return nbyte which have sent, otherwise return -1*/
int ee24cxxx_write_page(uint16_t addr, int len, uint8_t *buf)
{
  int rv=0; /* return value */
  uint16_t endaddr;

  /*if the rest space of page is sufficient to store len byte*/
  if ((addr + len) <= (addr | (PAGE_SIZE - 1)))
    endaddr = addr + len;
  /*else the rest space is deficient/insufficient/inadequate to store len byte*/
  else
    endaddr = (addr | (PAGE_SIZE - 1)) + 1;
  /*adjust len*/
  len = endaddr - addr;

 begin:
  TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN); /* send start condition */
  while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
  switch ((twst = TW_STATUS))
    {
    case TW_REP_START: /* OK, but should not happen */
    case TW_START:
      break;
    case TW_MT_ARB_LOST:
      goto begin;
    default:
      return -1;
      /*do /not/ send stop condition becasue I have sent nothing.*/
    }
  
  TWDR = SLA_W_24CXXX;
  TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
  while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
  switch ((twst = TW_STATUS))
    {
    case TW_MT_SLA_ACK:
      break;
    case TW_MT_SLA_NACK: /* nack during select: device busy writing */
      goto begin;
    case TW_MT_ARB_LOST: /* re-arbitrate */
      goto begin;
    default:
      rv=-1;
      goto quit; /*must send stop condition*/
    }
  TWDR = addr>>8; /* first address word (high)*/
  TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
  while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
  switch ((twst = TW_STATUS))
    {
    case TW_MT_DATA_ACK:
      break;
    case TW_MT_DATA_NACK:
      goto quit;
    case TW_MT_ARB_LOST:
      goto begin;
    default:
      rv=-1;
      goto quit;
    }
  TWDR = addr&0x00ff; /*second address word (low)*/
  TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
  while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
  switch ((twst = TW_STATUS))
    {
    case TW_MT_DATA_ACK:
      break;
    case TW_MT_DATA_NACK:
      goto quit;
    case TW_MT_ARB_LOST:
      goto begin;
    default:
      rv=-1;
      goto quit;
    }
  
  for (; len > 0; len--)
    {
      /* the address word low 6 bits are internally incremented following
	 the receipt of each data word. --at24c128/256 datasheet.
      */
      TWDR = *buf; /* send a byte */
      buf++;
      TWCR = _BV(TWINT) | _BV(TWEN); /* start transmission */
      while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
      switch ((twst = TW_STATUS))
	{
	case TW_MT_DATA_NACK:
	  rv=-1;
	  goto quit;
	case TW_MT_DATA_ACK:
	  rv++;
	  break;
	default:
	  rv=-1;
	  goto quit;
	}
    }
  
 quit:
  TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN); /* send stop condition */
  return rv;
  
}

/* also use page write mode */
int ee24cxxx_write_bytes(uint16_t addr, int len, uint8_t *buf)
{
  int rv;
  int total=0;
  
  do
    {
      rv = ee24cxxx_write_page(addr, len, buf);
      /*
	1)rv==-1: error.
	2)rv==0: retry.
	3)rv<len: need to store the rest bytes to a new page.
	4)rv==len: all bytes have been stored in this page. so I can return now.
      */
      if(rv==-1)
	return -1;
      len -= rv;      
      addr += rv;
      buf += rv;
      total += rv;
    }
  while(len > 0);
  
  return total;
      
}

/*random/sequential read mode*/
int ee24cxxx_read_bytes(uint16_t addr, int len, uint8_t *buf)
{
  uint8_t twcr;
  int rv=0;
  
 begin:
  TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN); /* send start condition */
  while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
  switch ((twst = TW_STATUS))
    {
    case TW_REP_START: /* OK, but should not happen */
    case TW_START:
      break;
    case TW_MT_ARB_LOST:
      goto begin;
    default:
      return -1;
      /*do /not/ send stop condition*/
    }
  
  TWDR = SLA_W_24CXXX;
  TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
  while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
  switch ((twst = TW_STATUS))
    {
    case TW_MT_SLA_ACK:
      break;
    case TW_MT_SLA_NACK: /* nack during select: device busy writing */
      goto begin;
    case TW_MT_ARB_LOST: /* re-arbitrate */
      goto begin;
    default:
      rv=-1;
      goto quit; /*return error and send stop condition*/
    }

  TWDR = addr>>8; /* first address word (high)*/
  TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
  while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
  switch ((twst = TW_STATUS))
    {
    case TW_MT_DATA_ACK:
      break;
    case TW_MT_DATA_NACK:
      goto quit;
    case TW_MT_ARB_LOST:
      goto begin;
    default:
      rv=-1;
      goto quit;
    }
  TWDR = addr&0x00ff; /*second address word (low)*/
  TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
  while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
  switch ((twst = TW_STATUS))
    {
    case TW_MT_DATA_ACK:
      break;
    case TW_MT_DATA_NACK:
      goto quit;
    case TW_MT_ARB_LOST:
      goto begin;
    default:
      rv=-1;
      goto quit;
    }
  

  /*Next cycle: master receiver mode*/
  TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN); /* send (rep.) start condition */
  while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
  switch ((twst = TW_STATUS))
    {
    case TW_START: /* OK, but should not happen */
    case TW_REP_START:
      break;
    case TW_MT_ARB_LOST:
      goto begin;
    default:
      rv=-1;
      goto quit;
    }
  
  TWDR = SLA_R_24CXXX;
  TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
  while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
  switch ((twst = TW_STATUS))
    {
    case TW_MR_SLA_ACK:
      break;
    case TW_MR_SLA_NACK:
      goto quit;
    case TW_MR_ARB_LOST:
      goto begin;
    default:
      rv=-1;
      goto quit;
    }

  /*
    1)len=1: after receiving a byte, the Master Receiver send NACK, then send stop.
    2)len>1: after receiving a byte, the Master Receiver send ACK, so the eeprom
    will send the following byte.
    about TWEA bit, see also at24c128/256 and atmega48 datasheet for details */  
  twcr = _BV(TWINT) | _BV(TWEN) | _BV(TWEA); /* MR send ACK */
  for(; len>0; len--)
    {
      if(len==1)
	twcr = _BV(TWINT) | _BV(TWEN); /* send NACK */
      
      TWCR=twcr; /* clear interrupt to start transmission */
      
      while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
      switch ((twst = TW_STATUS))
	{
	case TW_MR_DATA_NACK:
	  *buf = TWDR;
	  rv++;
	  len=0; /*just ensure*/
	  break;
	case TW_MR_DATA_ACK:
	  *buf = TWDR;
	  buf++;
	  rv++;
	  break;
	default:
	  rv=-1;
	  goto quit;
	  
	}
    }
 quit:
  TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN); /* send stop condition */
  return rv;
  
}