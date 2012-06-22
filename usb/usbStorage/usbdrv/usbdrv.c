/* ���: usbdrv.c
 * ������: ������� AVR USB
 * �����: Christian Starkjohann
 * �������: microsin.ru  
 * ���� ��������: 2004-12-29
 * ���������: 4
 * Copyright: (c) 2005 by OBJECTIVE DEVELOPMENT Software GmbH
 * ��������: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
 * �������: $Id: usbdrv.c 591 2008-05-03 20:21:19Z cs $
 */

#include "iarcompat.h"
#ifndef __IAR_SYSTEMS_ICC__
#   include <avr/io.h>
#   include <avr/pgmspace.h>
#endif
#include "usbdrv.h"
#include "oddebug.h"

/*
�������� ��������:
���� ������ ��������� ����� �������� USB �� ����� C. ��. usbdrv.h �� ������ ������������ �� �������� � �����.
*/

/* ------------------------------------------------------------------------- */

/* �������� �������� USB / ��������� � ������������� ����: */
uchar usbRxBuf[2*USB_BUFSIZE];  /* �������� ����� RX: PID, 8 ���� ������, 2 ����� CRC */
uchar       usbInputBufOffset;  /* �������� � usbRxBuf, ������������ ��� ������ �� ������ ������ */
uchar       usbDeviceAddr;      /* ����������� �� ����� ����������, �� ��������� 0 */
uchar       usbNewDeviceAddr;   /* ID ����������, ������� ��������������� ����� ���� ������� */
uchar       usbConfiguration;   /* ��������� � ��������� ������ ������������. ���������������� ���������, �� �� ������������ */
volatile schar usbRxLen;        /* = 0; ���������� ���� � usbRxBuf; 0 ��������, �������� ����, -1 ������������ ��� ���������� ������� */
uchar       usbCurrentTok;      /* ��������� �������� ����� ��� ����� �������� ����� ��� ���������� ������ OUT, ���� != 0 */
uchar       usbRxToken;         /* ����� �������� ���� ������; ��� ����� �������� ����� ��� ���������� OUT */
volatile uchar usbTxLen = USBPID_NAK;   /* ���������� ���� ��� �������� �� ��������� ������� IN ��� ������� ����������� (handshake) */
uchar       usbTxBuf[USB_BUFSIZE];/* ������ ��� �������� �� ��������� IN, �������� ���� usbTxLen �������� ����� ����������� (handshake) */
#if USB_COUNT_SOF
volatile uchar  usbSofCount;    /* ������������� �� 1 ������������ ������� ��� ������ SOF (Start-Of-Frame) */
#endif
#if USB_CFG_HAVE_INTRIN_ENDPOINT
usbTxStatus_t  usbTxStatus1;
#   if USB_CFG_HAVE_INTRIN_ENDPOINT3
usbTxStatus_t  usbTxStatus3;
#   endif
#endif

/* �������� ������� USB  / �� ������������ ��������� � ����� �� ���������� */
uchar               *usbMsgPtr;     					/* ������ ��� ����������� �������� -- ����� ROM ��� RAM */
static usbMsgLen_t  usbMsgLen = USB_NO_MSG; 	/* ���������� ���������� ���� */
static uchar        usbMsgFlags;    					/* �������� ������ - ��. ����� */

#define USB_FLG_MSGPTR_IS_ROM   (1<<6)
#define USB_FLG_USE_USER_RW     (1<<7)

/*
��������������� �������:
- �� ������� post/pre inc/dec �������� integer � ���������
- �������� �������� PRG_RDB() �� ����������� ���������� � �� ����������� side-������� � ����������
- ����������� ������������ �������� ����������, ������� ������ ���� � ��������� X/Y/Z
- ���������� ��������� � �������� ������ char � ����������, ����� ������������� 8-��� ����������
*/

/* -------------------------- String Descriptors --------------------------- */

#if USB_CFG_DESCR_PROPS_STRINGS == 0

#if USB_CFG_DESCR_PROPS_STRING_0 == 0
#undef USB_CFG_DESCR_PROPS_STRING_0
#define USB_CFG_DESCR_PROPS_STRING_0    sizeof(usbDescriptorString0)
PROGMEM char usbDescriptorString0[] = { /* ��������� ����� */
    4,          /* sizeof(usbDescriptorString0): ����� ��������� � ������ */
    3,          /* ��� ����������� */
    0x09, 0x04, /* ������ ����� (0x0409 = US-English) */
};
#endif

#if USB_CFG_DESCR_PROPS_STRING_VENDOR == 0 && USB_CFG_VENDOR_NAME_LEN
#undef USB_CFG_DESCR_PROPS_STRING_VENDOR
#define USB_CFG_DESCR_PROPS_STRING_VENDOR   sizeof(usbDescriptorStringVendor)
PROGMEM int  usbDescriptorStringVendor[] = {
    USB_STRING_DESCRIPTOR_HEADER(USB_CFG_VENDOR_NAME_LEN),
    USB_CFG_VENDOR_NAME
};
#endif

#if USB_CFG_DESCR_PROPS_STRING_PRODUCT == 0 && USB_CFG_DEVICE_NAME_LEN
#undef USB_CFG_DESCR_PROPS_STRING_PRODUCT
#define USB_CFG_DESCR_PROPS_STRING_PRODUCT   sizeof(usbDescriptorStringDevice)
PROGMEM int  usbDescriptorStringDevice[] = {
    USB_STRING_DESCRIPTOR_HEADER(USB_CFG_DEVICE_NAME_LEN),
    USB_CFG_DEVICE_NAME
};
#endif

#if USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER == 0 && USB_CFG_SERIAL_NUMBER_LEN
#undef USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER
#define USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER    sizeof(usbDescriptorStringSerialNumber)
PROGMEM int usbDescriptorStringSerialNumber[] = {
    USB_STRING_DESCRIPTOR_HEADER(USB_CFG_SERIAL_NUMBER_LEN),
    USB_CFG_SERIAL_NUMBER
};
#endif

#endif  /* USB_CFG_DESCR_PROPS_STRINGS == 0 */

/* --------------------------- ���������� ���������� --------------------------- */

#if USB_CFG_DESCR_PROPS_DEVICE == 0
#undef USB_CFG_DESCR_PROPS_DEVICE
#define USB_CFG_DESCR_PROPS_DEVICE  sizeof(usbDescriptorDevice)
PROGMEM char usbDescriptorDevice[] = {    /* USB ���������� ���������� */
    18,         /* sizeof(usbDescriptorDevice): ����� ���������� � ������ */
    USBDESCR_DEVICE,        /* ��� ����������� */
    0x10, 0x01,             /* �������������� ������ USB */
    USB_CFG_DEVICE_CLASS,
    USB_CFG_DEVICE_SUBCLASS,
    0,                      /* �������� */
    8,                      /* max ������ ������ */
    /* ��������� ��� �������������� ���� (cast) ������ ������ �� ������ ���� ���������, ��
     * ��� ����� ��� ����, ����� �������� �������������� (warning) � ���������� �� ���������.
     */
    (char)USB_CFG_VENDOR_ID,	/* 2 ����� */
    (char)USB_CFG_DEVICE_ID,	/* 2 ����� */
    USB_CFG_DEVICE_VERSION, 	/* 2 ����� */
    USB_CFG_DESCR_PROPS_STRING_VENDOR != 0 ? 1 : 0,         /* ������ ������ ������������� */
    USB_CFG_DESCR_PROPS_STRING_PRODUCT != 0 ? 2 : 0,        /* ������ ������ �������� */
    USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER != 0 ? 3 : 0,  /* ������ ������ ��������� ������ */
    1,          																						/* ���������� ������������ */
};
#endif

/* ----------------------- ���������� ������������ ------------------------ */

#if USB_CFG_DESCR_PROPS_HID_REPORT != 0 && USB_CFG_DESCR_PROPS_HID == 0
#undef USB_CFG_DESCR_PROPS_HID
#define USB_CFG_DESCR_PROPS_HID     9   /* ����� HID ����������� � ����������� ���� */
#endif

#if USB_CFG_DESCR_PROPS_CONFIGURATION == 0
#undef USB_CFG_DESCR_PROPS_CONFIGURATION
#define USB_CFG_DESCR_PROPS_CONFIGURATION   sizeof(usbDescriptorConfiguration)
PROGMEM char usbDescriptorConfiguration[] = {    /* USB ���������� ������������ */
    9,          /* sizeof(usbDescriptorConfiguration): ����� ���������� � ������ */
    USBDESCR_CONFIG,    /* ��� ����������� */
    18 + 7 * USB_CFG_HAVE_INTRIN_ENDPOINT + 7 * USB_CFG_HAVE_INTRIN_ENDPOINT3 +
                (USB_CFG_DESCR_PROPS_HID & 0xff), 0,
                /* ����� ����� ������������ ������ (������� ���������� �����������) */
    1,          /* ���������� ����������� � ���� ������������ */
    1,          /* ������ ���� ������������ */
    0,          /* ������ ������ ����� ������������ */
#if USB_CFG_IS_SELF_POWERED
    USBATTR_SELFPOWER,      /* �������� */
#else
    (char)USBATTR_BUSPOWER, /* �������� */
#endif
    USB_CFG_MAX_BUS_POWER/2,            /* max ��� USB � �������� 2mA */
/* ���������� ���������� ������� ���������� (inline): */
    9,          /* sizeof(usbDescrInterface): ����� ����������� � ������ */
    USBDESCR_INTERFACE, /* ��� ����������� */
    0,          /* ������ ����� ���������� */
    0,          /* �������������� ��������� ����� ���������� */
    USB_CFG_HAVE_INTRIN_ENDPOINT + USB_CFG_HAVE_INTRIN_ENDPOINT3, /* �������� ����� �� ����������� 0: ���������� ��������� ���������� �������� ����� */
    USB_CFG_INTERFACE_CLASS,
    USB_CFG_INTERFACE_SUBCLASS,
    USB_CFG_INTERFACE_PROTOCOL,
    0,          /* ������ ������ ��� ���������� */
#if (USB_CFG_DESCR_PROPS_HID & 0xff)    /* HID ���������� */
    9,          /* sizeof(usbDescrHID): ����� ����������� � ������ */
    USBDESCR_HID,   /* ��� �����������: HID */
    0x01, 0x01, /* BCD ������������� ������ HID */
    0x00,       /* ��� ������� ������ */
    0x01,       /* ����� ���������� ����������� ��������������� ������� HID (��� ������� ������ HID) */
    0x22,       /* ��� �����������: ������ */
    USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH, 0,  /* ����� ����� ����������� ������� */
#endif
#if USB_CFG_HAVE_INTRIN_ENDPOINT    /* ���������� �������� ����� ��� �������� ����� 1 */
    7,          /* sizeof(usbDescrEndpoint) */
    USBDESCR_ENDPOINT,  /* ��� ����������� = �������� ����� */
    (char)0x81, /* IN endpoint ����� 1 */
    0x03,       /* �������: �������� ����� � ����������� */
    8, 0,       /* max ������ ������ */
    USB_CFG_INTR_POLL_INTERVAL, /* � ms */
#endif
#if USB_CFG_HAVE_INTRIN_ENDPOINT3   /* ���������� �������� ����� ��� �������� ����� 3 */
    7,          /* sizeof(usbDescrEndpoint) */
    USBDESCR_ENDPOINT,  /* ��� ����������� = �������� ����� */
    (char)0x83, /* IN endpoint ����� 3 */
    0x03,       /* �������: �������� ����� � ����������� */
    8, 0,       /* max ������ ������ */
    USB_CFG_INTR_POLL_INTERVAL, /* � ms */
#endif
};
#endif

/* ------------------------------------------------------------------------- */

/* �� �� ���������� prog_int ��� prog_int16_t ��� ������������� � ���������� 
 *  �������� libc. ����� ������������ ������ ��� ��� �������������:
 */
#ifndef PRG_RDB
#define PRG_RDB(addr)   pgm_read_byte(addr)
#endif

/* ------------------------------------------------------------------------- */

static inline void  usbResetDataToggling(void)
{
#if USB_CFG_HAVE_INTRIN_ENDPOINT
    USB_SET_DATATOKEN1(USB_INITIAL_DATATOKEN);  /* ����� ������������ ������ ��� �������� ����� ���������� */
#   if USB_CFG_HAVE_INTRIN_ENDPOINT3
    USB_SET_DATATOKEN3(USB_INITIAL_DATATOKEN);  /* ����� ������������ ������ ��� �������� ����� ���������� */
#   endif
#endif
}

static inline void  usbResetStall(void)
{
#if USB_CFG_IMPLEMENT_HALT && USB_CFG_HAVE_INTRIN_ENDPOINT
        usbTxLen1 = USBPID_NAK;
#if USB_CFG_HAVE_INTRIN_ENDPOINT3
        usbTxLen3 = USBPID_NAK;
#endif
#endif
}

/* ------------------------------------------------------------------------- */

#if USB_CFG_HAVE_INTRIN_ENDPOINT
static void usbGenericSetInterrupt(uchar *data, uchar len, usbTxStatus_t *txStatus)
{
uchar   *p;
char    i;

#if USB_CFG_IMPLEMENT_HALT
    if(usbTxLen1 == USBPID_STALL)
        return;
#endif
    if(txStatus->len & 0x10){   /* ����� ������ ��� ���� */
        txStatus->buffer[0] ^= USBPID_DATA0 ^ USBPID_DATA1; /* ������������ ������ */
    }else{
        txStatus->len = USBPID_NAK; /* �������� �������� ������������ (��������������) ������ ���������� */
    }
    p = txStatus->buffer + 1;
    i = len;
    do{                         /* ���� len == 0, �� ��� ����� �������� 1 ����, �� ��� �� �������� */
        *p++ = *data++;
    }while(--i > 0);            /* ���������� ������ � ����� ���������� �� 2 ����� ������, ��� � ������ */
    usbCrc16Append(&txStatus->buffer[1], len);
    txStatus->len = len + 4;    /* len ������ �������� ���� ������������� (sync byte) */
    DBG2(0x21 + (((int)txStatus >> 3) & 3), txStatus->buffer, len + 3);
}

USB_PUBLIC void usbSetInterrupt(uchar *data, uchar len)
{
    usbGenericSetInterrupt(data, len, &usbTxStatus1);
}
#endif

#if USB_CFG_HAVE_INTRIN_ENDPOINT3
USB_PUBLIC void usbSetInterrupt3(uchar *data, uchar len)
{
    usbGenericSetInterrupt(data, len, &usbTxStatus3);
}
#endif

/* ------------------ ������� ��� ����, ���������� ����� ------------------- */

/* ����������� ��������� define ��� ��������� switch statement, ����� ������� �����
 *  ������������ if()else if() � switch/case. �������� switch() ����� ����������
 *  ��� ������� ������������������� ������, if() ����� � ������ �������.
 */
#if USB_CFG_USE_SWITCH_STATEMENT
#   define SWITCH_START(cmd)       switch(cmd){{
#   define SWITCH_CASE(value)      }break; case (value):{
#   define SWITCH_CASE2(v1,v2)     }break; case (v1): case(v2):{
#   define SWITCH_CASE3(v1,v2,v3)  }break; case (v1): case(v2): case(v3):{
#   define SWITCH_DEFAULT          }break; default:{
#   define SWITCH_END              }}
#else
#   define SWITCH_START(cmd)       {uchar _cmd = cmd; if(0){
#   define SWITCH_CASE(value)      }else if(_cmd == (value)){
#   define SWITCH_CASE2(v1,v2)     }else if(_cmd == (v1) || _cmd == (v2)){
#   define SWITCH_CASE3(v1,v2,v3)  }else if(_cmd == (v1) || _cmd == (v2) || (_cmd == v3)){
#   define SWITCH_DEFAULT          }else{
#   define SWITCH_END              }}
#endif

#ifndef USB_RX_USER_HOOK
#define USB_RX_USER_HOOK(data, len)
#endif
#ifndef USB_SET_ADDRESS_HOOK
#define USB_SET_ADDRESS_HOOK()
#endif

/* ------------------------------------------------------------------------- */

/* �� ���������� if() ������ #if � ������� �����, ������ ��� #if �� ����� ��������������
 *  � ������� � ���������� ��� ��� ����� ������������ ���������.
 * ��� ����� ��������� �������� � ��������������� ��������� (undefined symbols), ���� 
 *  ���������� ���������� ��� �����������!
 */
#define GET_DESCRIPTOR(cfgProp, staticName)         \
    if(cfgProp){                                    \
        if((cfgProp) & USB_PROP_IS_RAM)             \
            flags = 0;                              \
        if((cfgProp) & USB_PROP_IS_DYNAMIC){        \
            len = usbFunctionDescriptor(rq);        \
        }else{                                      \
            len = USB_PROP_LENGTH(cfgProp);         \
            usbMsgPtr = (uchar *)(staticName);      \
        }                                           \
    }

/* usbDriverDescriptor() ������������ usbFunctionDescriptor(), �� ������������
 * ��������� ��� ���� ����� ������������.
 */
static inline usbMsgLen_t usbDriverDescriptor(usbRequest_t *rq)
{
usbMsgLen_t len = 0;
uchar       flags = USB_FLG_MSGPTR_IS_ROM;

    SWITCH_START(rq->wValue.bytes[1])
    SWITCH_CASE(USBDESCR_DEVICE)    /* 1 */
        GET_DESCRIPTOR(USB_CFG_DESCR_PROPS_DEVICE, usbDescriptorDevice)
    SWITCH_CASE(USBDESCR_CONFIG)    /* 2 */
        GET_DESCRIPTOR(USB_CFG_DESCR_PROPS_CONFIGURATION, usbDescriptorConfiguration)
    SWITCH_CASE(USBDESCR_STRING)    /* 3 */
#if USB_CFG_DESCR_PROPS_STRINGS & USB_PROP_IS_DYNAMIC
        if(USB_CFG_DESCR_PROPS_STRINGS & USB_PROP_IS_RAM)
            flags = 0;
        len = usbFunctionDescriptor(rq);
#else   /* USB_CFG_DESCR_PROPS_STRINGS & USB_PROP_IS_DYNAMIC */
        SWITCH_START(rq->wValue.bytes[0])
        SWITCH_CASE(0)
            GET_DESCRIPTOR(USB_CFG_DESCR_PROPS_STRING_0, usbDescriptorString0)
        SWITCH_CASE(1)
            GET_DESCRIPTOR(USB_CFG_DESCR_PROPS_STRING_VENDOR, usbDescriptorStringVendor)
        SWITCH_CASE(2)
            GET_DESCRIPTOR(USB_CFG_DESCR_PROPS_STRING_PRODUCT, usbDescriptorStringDevice)
        SWITCH_CASE(3)
            GET_DESCRIPTOR(USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER, usbDescriptorStringSerialNumber)
        SWITCH_DEFAULT
            if(USB_CFG_DESCR_PROPS_UNKNOWN & USB_PROP_IS_DYNAMIC){
                len = usbFunctionDescriptor(rq);
            }
        SWITCH_END
#endif  /* USB_CFG_DESCR_PROPS_STRINGS & USB_PROP_IS_DYNAMIC */
#if USB_CFG_DESCR_PROPS_HID_REPORT  /* ���� ���������, ������������ ������ ����������� HID */
    SWITCH_CASE(USBDESCR_HID)       /* 0x21 */
        GET_DESCRIPTOR(USB_CFG_DESCR_PROPS_HID, usbDescriptorConfiguration + 18)
    SWITCH_CASE(USBDESCR_HID_REPORT)/* 0x22 */
        GET_DESCRIPTOR(USB_CFG_DESCR_PROPS_HID_REPORT, usbDescriptorHidReport)
#endif
    SWITCH_DEFAULT
        if(USB_CFG_DESCR_PROPS_UNKNOWN & USB_PROP_IS_DYNAMIC){
            len = usbFunctionDescriptor(rq);
        }
    SWITCH_END
    usbMsgFlags = flags;
    return len;
}

/* ------------------------------------------------------------------------- */

/* usbDriverSetup() ������������ usbFunctionSetup(), �� ������������ ��� 
 * ����������� �������� ������ ������ � ����������� ��������.
 */
static inline usbMsgLen_t usbDriverSetup(usbRequest_t *rq)
{
uchar   len  = 0, *dataPtr = usbTxBuf + 9;  /* ����� 2 ����� ���������� ������������ � ����� ������ */
uchar   value = rq->wValue.bytes[0];
#if USB_CFG_IMPLEMENT_HALT
uchar   index = rq->wIndex.bytes[0];
#endif

    dataPtr[0] = 0; /* ����� ����� �� ��������� �� USBRQ_GET_STATUS � USBRQ_GET_INTERFACE */
    SWITCH_START(rq->bRequest)
    SWITCH_CASE(USBRQ_GET_STATUS)           /* 0 */
        uchar recipient = rq->bmRequestType & USBRQ_RCPT_MASK;  /* ��������� �������������� ops ����������, ����� ���������� ������ � ������ */
        if(USB_CFG_IS_SELF_POWERED && recipient == USBRQ_RCPT_DEVICE)
            dataPtr[0] =  USB_CFG_IS_SELF_POWERED;
#if USB_CFG_IMPLEMENT_HALT
        if(recipient == USBRQ_RCPT_ENDPOINT && index == 0x81)   /* ������ ������� ��� �������� ����� 1 */
            dataPtr[0] = usbTxLen1 == USBPID_STALL;
#endif
        dataPtr[1] = 0;
        len = 2;
#if USB_CFG_IMPLEMENT_HALT
    SWITCH_CASE2(USBRQ_CLEAR_FEATURE, USBRQ_SET_FEATURE)    /* 1, 3 */
        if(value == 0 && index == 0x81){    /* ����������� (feature) 0 == HALT ��� �������� ����� == 1 */
            usbTxLen1 = rq->bRequest == USBRQ_CLEAR_FEATURE ? USBPID_NAK : USBPID_STALL;
            usbResetDataToggling();
        }
#endif
    SWITCH_CASE(USBRQ_SET_ADDRESS)          /* 5 */
        usbNewDeviceAddr = value;
        USB_SET_ADDRESS_HOOK();
    SWITCH_CASE(USBRQ_GET_DESCRIPTOR)       /* 6 */
        len = usbDriverDescriptor(rq);
        goto skipMsgPtrAssignment;
    SWITCH_CASE(USBRQ_GET_CONFIGURATION)    /* 8 */
        dataPtr = &usbConfiguration;  /* �������� �������� ������� ������������ */
        len = 1;
    SWITCH_CASE(USBRQ_SET_CONFIGURATION)    /* 9 */
        usbConfiguration = value;
        usbResetStall();
    SWITCH_CASE(USBRQ_GET_INTERFACE)        /* 10 */
        len = 1;
#if USB_CFG_HAVE_INTRIN_ENDPOINT
    SWITCH_CASE(USBRQ_SET_INTERFACE)        /* 11 */
        usbResetDataToggling();
        usbResetStall();
#endif
    SWITCH_DEFAULT                          /* 7=SET_DESCRIPTOR, 12=SYNC_FRAME */
        /* ������ �� �������� ����� �������������� ���? */
    SWITCH_END
    usbMsgPtr = dataPtr;
skipMsgPtrAssignment:
    return len;
}

/* ------------------------------------------------------------------------- */

/* usbProcessRx() ���������� ��� ������� ���������, ��������� �������������
 *  ����������. ��� ��������� ������ SETUP � DATA � ������������ �� 
 *  ��������������.
 */
static inline void usbProcessRx(uchar *data, uchar len)
{
usbRequest_t    *rq = (void *)data;

/* usbRxToken ����� ����:
 * 0x2d 00101101 (USBPID_SETUP ��� ������ setup)
 * 0xe1 11100001 (USBPID_OUT: ���� ������ �������� setup)
 * 0...0x0f ��� OUT �� �������� ����� X
 */
    DBG2(0x10 + (usbRxToken & 0xf), data, len); /* SETUP=1d, SETUP-DATA=11, OUTx=1x */
    USB_RX_USER_HOOK(data, len)
#if USB_CFG_IMPLEMENT_FN_WRITEOUT
    if(usbRxToken < 0x10){  /* OUT ��� �������� ����� != 0: ����� �������� ����� ��������� � usbRxToken */
        usbFunctionWriteOut(data, len);
        return;
    }
#endif
    if(usbRxToken == (uchar)USBPID_SETUP){
        if(len != 8)    /* ������ setup ������ ���� ������ 8 ����. ����� ������������. */
            return;
        usbMsgLen_t replyLen;
        usbTxBuf[0] = USBPID_DATA0;         /* ������������� ������������ ������ */
        usbTxLen = USBPID_NAK;              /* ��������� ��������� �������� */
        usbMsgFlags = 0;
        uchar type = rq->bmRequestType & USBRQ_TYPE_MASK;
        if(type != USBRQ_TYPE_STANDARD){    /* ����������� ������� ������������� ��������� */
            replyLen = usbFunctionSetup(data);
        }else{
            replyLen = usbDriverSetup(rq);
        }
#if USB_CFG_IMPLEMENT_FN_READ || USB_CFG_IMPLEMENT_FN_WRITE
        if(replyLen == USB_NO_MSG){         /* ������������ ��������������� ������������� ������� ������/������ */
            /* ������ ��������� �������� ������� �� replyLen */
            if((rq->bmRequestType & USBRQ_DIR_MASK) != USBRQ_DIR_HOST_TO_DEVICE){
                replyLen = rq->wLength.bytes[0];    /* ������ �������� IN */
            }
            usbMsgFlags = USB_FLG_USE_USER_RW;
        }else   /* 'else' ������������� ����� replyLen USB_NO_MSG ��� ������������ ����� ��������. */
#endif
        if(sizeof(replyLen) < sizeof(rq->wLength.word)){ /* �������� ����������� � ������������ */
            if(!rq->wLength.bytes[1] && replyLen > rq->wLength.bytes[0])    /* ���������� ����� �� max */
                replyLen = rq->wLength.bytes[0];
        }else{
            if(replyLen > rq->wLength.word)     														/* ���������� ����� �� max */
                replyLen = rq->wLength.word;
        }
        usbMsgLen = replyLen;
    }else{  /* usbRxToken ������ ���� USBPID_OUT, ��� �������� ���� ������ setup (control-out) */
#if USB_CFG_IMPLEMENT_FN_WRITE
        if(usbMsgFlags & USB_FLG_USE_USER_RW){
            uchar rval = usbFunctionWrite(data, len);
            if(rval == 0xff){   /* ��������� ������ */
                usbTxLen = USBPID_STALL;
            }else if(rval != 0){    /* ��� ��������� ����� */
                usbMsgLen = 0;  /* ����� ������� ������� ����� */
            }
        }
#endif
    }
}

/* ------------------------------------------------------------------------- */

/* ��� ������� ������������ usbFunctionRead(), �� ��� ����� ���������� ��� 
 *  ������, ������������� �������������� ��������� (��������, ������ �����������).
 */
static uchar usbDeviceRead(uchar *data, uchar len)
{
    if(len > 0){    /* �� ��������� ���������� �������� �������� ������� */
#if USB_CFG_IMPLEMENT_FN_READ
        if(usbMsgFlags & USB_FLG_USE_USER_RW){
            len = usbFunctionRead(data, len);
        }else
#endif
        {
            uchar i = len, *r = usbMsgPtr;
            if(usbMsgFlags & USB_FLG_MSGPTR_IS_ROM){    /* ������ ROM */
                do{
                    uchar c = PRG_RDB(r);    /* ��������� ���������� char ��� ��������� �������� �������� */
                    *data++ = c;
                    r++;
                }while(--i);
            }else{  /* ������ RAM */
                do{
                    *data++ = *r++;
                }while(--i);
            }
            usbMsgPtr = r;
        }
    }
    return len;
}

/* ------------------------------------------------------------------------- */

/* usbBuildTxBlock() ����������, ����� �� ����� ������ ��� ��������, � �����
 *  ������������ ����� �������� ������������ ����������.
 */
static inline void usbBuildTxBlock(void)
{
usbMsgLen_t wantLen;
uchar       len;

    wantLen = usbMsgLen;
    if(wantLen > 8)
        wantLen = 8;
    usbMsgLen -= wantLen;
    usbTxBuf[0] ^= USBPID_DATA0 ^ USBPID_DATA1; /* ������������ DATA */
    len = usbDeviceRead(usbTxBuf + 1, wantLen);
    if(len <= 8){           /* ���������� ����� ������ */
        usbCrc16Append(&usbTxBuf[1], len);
        len += 4;           /* ����� ������� ���� sync */
        if(len < 12)        /* ����� ������ �������������� ����� ��������� */
            usbMsgLen = USB_NO_MSG;
    }else{
        len = USBPID_STALL;   /* ��������� �������� ����� */
        usbMsgLen = USB_NO_MSG;
    }
    usbTxLen = len;
    DBG2(0x20, usbTxBuf, len-1);
}

/* ------------------------------------------------------------------------- */

static inline void usbHandleResetHook(uchar notResetState)
{
#ifdef USB_RESET_HOOK
static uchar    wasReset;
uchar           isReset = !notResetState;

    if(wasReset != isReset){
        USB_RESET_HOOK(isReset);
        wasReset = isReset;
    }
#endif
}

/* ------------------------------------------------------------------------- */

USB_PUBLIC void usbPoll(void)
{
schar   len;
uchar   i;

    len = usbRxLen - 3;
    if(len >= 0){
/* ����� �� ������ ��������� CRC16 -- �� ACK ��� ����� ����������. ���� ���
 *  ���������� �������� ����������� ������ � ���� ��������, ���������� CRC � ���� ������
 *  ���������� � ��������� �� ������� ������� �����. ��������� ACK ���������� ��-������,
 *  ������� ��� ������� ������ ���� ���������� �� ������ ����������.
 * unsigned crc = usbCrc16(buffer + 1, usbRxLen - 3);
 */
        usbProcessRx(usbRxBuf + USB_BUFSIZE + 1 - usbInputBufOffset, len);
#if USB_CFG_HAVE_FLOWCONTROL
        if(usbRxLen > 0)    /* ���� �� ��������������, �������� ������ mark */
            usbRxLen = 0;
#else
        usbRxLen = 0;       /* mark rx ����� ��� �������� */
#endif
    }
    if(usbTxLen & 0x10){    /* �������� system idle */
        if(usbMsgLen != USB_NO_MSG){    /* ������ ��� �������� � ��������? */
            usbBuildTxBlock();
        }
    }
    for(i = 10; i > 0; i--){
        uchar usbLineStatus = USBIN & USBMASK;
        if(usbLineStatus != 0)  /* SE0 ���������� */
            break;
    }
    if(i == 0){ /* ��������� RESET, ���������� ��������� ��� �� ����� ������ */
        usbNewDeviceAddr = 0;
        usbDeviceAddr = 0;
        usbResetStall();
        DBG1(0xff, 0, 0);
    }
    usbHandleResetHook(i);
}

/* ------------------------------------------------------------------------- */

USB_PUBLIC void usbInit(void)
{
#if USB_INTR_CFG_SET != 0
    USB_INTR_CFG |= USB_INTR_CFG_SET;
#endif
#if USB_INTR_CFG_CLR != 0
    USB_INTR_CFG &= ~(USB_INTR_CFG_CLR);
#endif
    USB_INTR_ENABLE |= (1 << USB_INTR_ENABLE_BIT);
    usbResetDataToggling();
#if USB_CFG_HAVE_INTRIN_ENDPOINT
    usbTxLen1 = USBPID_NAK;
#if USB_CFG_HAVE_INTRIN_ENDPOINT3
    usbTxLen3 = USBPID_NAK;
#endif
#endif
}

/* ------------------------------------------------------------------------- */
