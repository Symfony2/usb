/* Имя: usbdrv.c
 * Проект: драйвер AVR USB
 * Автор: Christian Starkjohann
 * Перевод: microsin.ru  
 * Дата создания: 2004-12-29
 * Табуляция: 4
 * Copyright: (c) 2005 by OBJECTIVE DEVELOPMENT Software GmbH
 * Лицензия: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
 * Ревизия: $Id: usbdrv.c 591 2008-05-03 20:21:19Z cs $
 */

#include "iarcompat.h"
#ifndef __IAR_SYSTEMS_ICC__
#   include <avr/io.h>
#   include <avr/pgmspace.h>
#endif
#include "usbdrv.h"
#include "oddebug.h"

/*
Основное описание:
Этот модуль оеализует часть драйвера USB на языке C. См. usbdrv.h по поводу документации по драйверу в целом.
*/

/* ------------------------------------------------------------------------- */

/* двоичные регистры USB / интерфейс к ассемблерному коду: */
uchar usbRxBuf[2*USB_BUFSIZE];  /* двоичный буфер RX: PID, 8 байт данных, 2 байта CRC */
uchar       usbInputBufOffset;  /* смещение в usbRxBuf, используемое для приема на низком уровне */
uchar       usbDeviceAddr;      /* назначается во время энумерации, по умолчанию 0 */
uchar       usbNewDeviceAddr;   /* ID устройства, которое устанавливаться после фазы статуса */
uchar       usbConfiguration;   /* выбранная в настоящий момент конфигурация. Администрируется драйвером, но не используется */
volatile schar usbRxLen;        /* = 0; количество байт в usbRxBuf; 0 означает, чтобуфер пуст, -1 используется для управления потоком */
uchar       usbCurrentTok;      /* последний принятый токен или номер конечной точки для последнего токена OUT, если != 0 */
uchar       usbRxToken;         /* токен принятых нами данных; или номер конечной точки для последнего OUT */
volatile uchar usbTxLen = USBPID_NAK;   /* количество байт для передачи со следующим токеном IN или токеном рукопожатия (handshake) */
uchar       usbTxBuf[USB_BUFSIZE];/* данные для передачи со следующим IN, свободно если usbTxLen содержит токен рукопожатия (handshake) */
#if USB_COUNT_SOF
volatile uchar  usbSofCount;    /* увеличивается на 1 ассемблерным модулем при каждом SOF (Start-Of-Frame) */
#endif
#if USB_CFG_HAVE_INTRIN_ENDPOINT
usbTxStatus_t  usbTxStatus1;
#   if USB_CFG_HAVE_INTRIN_ENDPOINT3
usbTxStatus_t  usbTxStatus3;
#   endif
#endif

/* регистры статуса USB  / не используются совместно с кодом на ассемблере */
uchar               *usbMsgPtr;     					/* данные для последующей передачи -- адрес ROM или RAM */
static usbMsgLen_t  usbMsgLen = USB_NO_MSG; 	/* оставшееся количество байт */
static uchar        usbMsgFlags;    					/* значения флагов - см. далее */

#define USB_FLG_MSGPTR_IS_ROM   (1<<6)
#define USB_FLG_USE_USER_RW     (1<<7)

/*
оптимизационные заметки:
- не делайте post/pre inc/dec величины integer в операциях
- назначте значения PRG_RDB() на регистровые переменные и не используйте side-эффекты в аргументах
- используйте ограниченный диапазон переменных, которые должны быть в регистрах X/Y/Z
- назначайте выражения с размером данных char в переменные, чтобы задействовать 8-бит арифметику
*/

/* -------------------------- String Descriptors --------------------------- */

#if USB_CFG_DESCR_PROPS_STRINGS == 0

#if USB_CFG_DESCR_PROPS_STRING_0 == 0
#undef USB_CFG_DESCR_PROPS_STRING_0
#define USB_CFG_DESCR_PROPS_STRING_0    sizeof(usbDescriptorString0)
PROGMEM char usbDescriptorString0[] = { /* описатель языка */
    4,          /* sizeof(usbDescriptorString0): длина описателя в байтах */
    3,          /* тип дескриптора */
    0x09, 0x04, /* индекс языка (0x0409 = US-English) */
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

/* --------------------------- Дескриптор устройства --------------------------- */

#if USB_CFG_DESCR_PROPS_DEVICE == 0
#undef USB_CFG_DESCR_PROPS_DEVICE
#define USB_CFG_DESCR_PROPS_DEVICE  sizeof(usbDescriptorDevice)
PROGMEM char usbDescriptorDevice[] = {    /* USB дескриптор устройства */
    18,         /* sizeof(usbDescriptorDevice): длина устройства в байтах */
    USBDESCR_DEVICE,        /* тип дескриптора */
    0x10, 0x01,             /* поддерживаемая версия USB */
    USB_CFG_DEVICE_CLASS,
    USB_CFG_DEVICE_SUBCLASS,
    0,                      /* протокол */
    8,                      /* max размер пакета */
    /* следующие два преобразования типа (cast) влияют только на первый байт константы, но
     * это важно для того, чтобы избежать предупреждения (warning) с величинами по умолчанию.
     */
    (char)USB_CFG_VENDOR_ID,	/* 2 байта */
    (char)USB_CFG_DEVICE_ID,	/* 2 байта */
    USB_CFG_DEVICE_VERSION, 	/* 2 байта */
    USB_CFG_DESCR_PROPS_STRING_VENDOR != 0 ? 1 : 0,         /* индекс строки производителя */
    USB_CFG_DESCR_PROPS_STRING_PRODUCT != 0 ? 2 : 0,        /* индекс строки продукта */
    USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER != 0 ? 3 : 0,  /* индекс строки серийного номера */
    1,          																						/* количество конфигураций */
};
#endif

/* ----------------------- Дескриптор конфигурации ------------------------ */

#if USB_CFG_DESCR_PROPS_HID_REPORT != 0 && USB_CFG_DESCR_PROPS_HID == 0
#undef USB_CFG_DESCR_PROPS_HID
#define USB_CFG_DESCR_PROPS_HID     9   /* длина HID дескриптора в дескрипторе ниже */
#endif

#if USB_CFG_DESCR_PROPS_CONFIGURATION == 0
#undef USB_CFG_DESCR_PROPS_CONFIGURATION
#define USB_CFG_DESCR_PROPS_CONFIGURATION   sizeof(usbDescriptorConfiguration)
PROGMEM char usbDescriptorConfiguration[] = {    /* USB дескриптор конфигурации */
    9,          /* sizeof(usbDescriptorConfiguration): длина устройства в байтах */
    USBDESCR_CONFIG,    /* тип дескриптора */
    18 + 7 * USB_CFG_HAVE_INTRIN_ENDPOINT + 7 * USB_CFG_HAVE_INTRIN_ENDPOINT3 +
                (USB_CFG_DESCR_PROPS_HID & 0xff), 0,
                /* общая длина возвращаемых данных (включая встроенные дескрипторы) */
    1,          /* количество интерфейсов в этой конфигурации */
    1,          /* индекс этой конфигурации */
    0,          /* индекс строки имени конфигурации */
#if USB_CFG_IS_SELF_POWERED
    USBATTR_SELFPOWER,      /* атрибуты */
#else
    (char)USBATTR_BUSPOWER, /* атрибуты */
#endif
    USB_CFG_MAX_BUS_POWER/2,            /* max ток USB в единицах 2mA */
/* дескриптор интерфейса следует встроенным (inline): */
    9,          /* sizeof(usbDescrInterface): длина дескриптора в байтах */
    USBDESCR_INTERFACE, /* тип дескриптора */
    0,          /* индекс этого интерфейса */
    0,          /* альтернативная установка этого интерфейса */
    USB_CFG_HAVE_INTRIN_ENDPOINT + USB_CFG_HAVE_INTRIN_ENDPOINT3, /* конечные точки за исключением 0: количество следующих описателей конечных точек */
    USB_CFG_INTERFACE_CLASS,
    USB_CFG_INTERFACE_SUBCLASS,
    USB_CFG_INTERFACE_PROTOCOL,
    0,          /* индекс строки для интерфейса */
#if (USB_CFG_DESCR_PROPS_HID & 0xff)    /* HID дескриптор */
    9,          /* sizeof(usbDescrHID): длина дескриптора в байтах */
    USBDESCR_HID,   /* тип дескриптора: HID */
    0x01, 0x01, /* BCD представление версии HID */
    0x00,       /* код целевой страны */
    0x01,       /* номер следующего дескриптора информационного репорта HID (или другого класса HID) */
    0x22,       /* тип дескриптора: репорт */
    USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH, 0,  /* общая длина дескриптора репорта */
#endif
#if USB_CFG_HAVE_INTRIN_ENDPOINT    /* дескриптор конечной точки для конечной точки 1 */
    7,          /* sizeof(usbDescrEndpoint) */
    USBDESCR_ENDPOINT,  /* тип дескриптора = конечная точка */
    (char)0x81, /* IN endpoint номер 1 */
    0x03,       /* атрибут: конечная точка с прерыванием */
    8, 0,       /* max размер пакета */
    USB_CFG_INTR_POLL_INTERVAL, /* в ms */
#endif
#if USB_CFG_HAVE_INTRIN_ENDPOINT3   /* дескриптор конечной точки для конечной точки 3 */
    7,          /* sizeof(usbDescrEndpoint) */
    USBDESCR_ENDPOINT,  /* тип дескриптора = конечная точка */
    (char)0x83, /* IN endpoint номер 3 */
    0x03,       /* атрибут: конечная точка с прерыванием */
    8, 0,       /* max размер пакета */
    USB_CFG_INTR_POLL_INTERVAL, /* в ms */
#endif
};
#endif

/* ------------------------------------------------------------------------- */

/* Мы не используем prog_int или prog_int16_t для совместимости с различными 
 *  версиями libc. Здесь используется другой хак для совместимости:
 */
#ifndef PRG_RDB
#define PRG_RDB(addr)   pgm_read_byte(addr)
#endif

/* ------------------------------------------------------------------------- */

static inline void  usbResetDataToggling(void)
{
#if USB_CFG_HAVE_INTRIN_ENDPOINT
    USB_SET_DATATOKEN1(USB_INITIAL_DATATOKEN);  /* сброс переключения данных для конечной точки прерывания */
#   if USB_CFG_HAVE_INTRIN_ENDPOINT3
    USB_SET_DATATOKEN3(USB_INITIAL_DATATOKEN);  /* сброс переключения данных для конечной точки прерывания */
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
    if(txStatus->len & 0x10){   /* буфер пакета был пуст */
        txStatus->buffer[0] ^= USBPID_DATA0 ^ USBPID_DATA1; /* переключение токена */
    }else{
        txStatus->len = USBPID_NAK; /* избегаем отправки неактуальных (перезаписанных) данных прерывания */
    }
    p = txStatus->buffer + 1;
    i = len;
    do{                         /* если len == 0, мы все равно копируем 1 байт, но это не проблема */
        *p++ = *data++;
    }while(--i > 0);            /* управление циклом в конце составляет на 2 байта короче, чем в начале */
    usbCrc16Append(&txStatus->buffer[1], len);
    txStatus->len = len + 4;    /* len должна включать байт синхронизации (sync byte) */
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

/* ------------------ утилиты для кода, следующего далее ------------------- */

/* Испельзуйте операторы define для оператора switch statement, можно выбрать между
 *  реализациями if()else if() и switch/case. Оператор switch() более эффективен
 *  для БОЛЬШИХ последовательностей выбора, if() лучше в других случаях.
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

/* Мы используем if() вместо #if в макросе далее, потому что #if не может использоваться
 *  в макросе и компилятор так или иначе оптимизирует константы.
 * Это может создавать проблемы с неопределенными символами (undefined symbols), если 
 *  компиляция происходит без оптимизации!
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

/* usbDriverDescriptor() эквивалентна usbFunctionDescriptor(), но используется
 * внутренне для всех типов дескрипторов.
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
#if USB_CFG_DESCR_PROPS_HID_REPORT  /* если разрешено, поддерживает только дескрипторы HID */
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

/* usbDriverSetup() эквивалентна usbFunctionSetup(), но используется для 
 * стандартных запросов вместо класса и стандартных запросов.
 */
static inline usbMsgLen_t usbDriverSetup(usbRequest_t *rq)
{
uchar   len  = 0, *dataPtr = usbTxBuf + 9;  /* здесь 2 байта свободного пространства в конце буфера */
uchar   value = rq->wValue.bytes[0];
#if USB_CFG_IMPLEMENT_HALT
uchar   index = rq->wIndex.bytes[0];
#endif

    dataPtr[0] = 0; /* общий ответ по умолчанию на USBRQ_GET_STATUS и USBRQ_GET_INTERFACE */
    SWITCH_START(rq->bRequest)
    SWITCH_CASE(USBRQ_GET_STATUS)           /* 0 */
        uchar recipient = rq->bmRequestType & USBRQ_RCPT_MASK;  /* назначьте арифметический ops переменным, чтобы предписать размер в байтах */
        if(USB_CFG_IS_SELF_POWERED && recipient == USBRQ_RCPT_DEVICE)
            dataPtr[0] =  USB_CFG_IS_SELF_POWERED;
#if USB_CFG_IMPLEMENT_HALT
        if(recipient == USBRQ_RCPT_ENDPOINT && index == 0x81)   /* запрос статуса для конечной точки 1 */
            dataPtr[0] = usbTxLen1 == USBPID_STALL;
#endif
        dataPtr[1] = 0;
        len = 2;
#if USB_CFG_IMPLEMENT_HALT
    SWITCH_CASE2(USBRQ_CLEAR_FEATURE, USBRQ_SET_FEATURE)    /* 1, 3 */
        if(value == 0 && index == 0x81){    /* особенность (feature) 0 == HALT для конечной точки == 1 */
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
        dataPtr = &usbConfiguration;  /* отправка величины текущей конфигурации */
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
        /* Должны мы добавить здесь дополнительный хук? */
    SWITCH_END
    usbMsgPtr = dataPtr;
skipMsgPtrAssignment:
    return len;
}

/* ------------------------------------------------------------------------- */

/* usbProcessRx() вызывается для каждого сообщения, принятого подпрограммой
 *  прерывания. Она различает пакеты SETUP и DATA и обрабатывает их 
 *  соответственно.
 */
static inline void usbProcessRx(uchar *data, uchar len)
{
usbRequest_t    *rq = (void *)data;

/* usbRxToken может быть:
 * 0x2d 00101101 (USBPID_SETUP для данных setup)
 * 0xe1 11100001 (USBPID_OUT: фаза данных передачи setup)
 * 0...0x0f для OUT на конечной точке X
 */
    DBG2(0x10 + (usbRxToken & 0xf), data, len); /* SETUP=1d, SETUP-DATA=11, OUTx=1x */
    USB_RX_USER_HOOK(data, len)
#if USB_CFG_IMPLEMENT_FN_WRITEOUT
    if(usbRxToken < 0x10){  /* OUT для конечной точки != 0: номер конечной точки находится в usbRxToken */
        usbFunctionWriteOut(data, len);
        return;
    }
#endif
    if(usbRxToken == (uchar)USBPID_SETUP){
        if(len != 8)    /* Размер setup должен быть всегда 8 байт. Иначе игнорируется. */
            return;
        usbMsgLen_t replyLen;
        usbTxBuf[0] = USBPID_DATA0;         /* иницализируем переключение данных */
        usbTxLen = USBPID_NAK;              /* прерываем ожидающую передачу */
        usbMsgFlags = 0;
        uchar type = rq->bmRequestType & USBRQ_TYPE_MASK;
        if(type != USBRQ_TYPE_STANDARD){    /* стандартные запросы обрабатыватся драйвером */
            replyLen = usbFunctionSetup(data);
        }else{
            replyLen = usbDriverSetup(rq);
        }
#if USB_CFG_IMPLEMENT_FN_READ || USB_CFG_IMPLEMENT_FN_WRITE
        if(replyLen == USB_NO_MSG){         /* используется предоставляемая пользователем функция чтения/записи */
            /* делаем некоторое создание условий на replyLen */
            if((rq->bmRequestType & USBRQ_DIR_MASK) != USBRQ_DIR_HOST_TO_DEVICE){
                replyLen = rq->wLength.bytes[0];    /* только передачи IN */
            }
            usbMsgFlags = USB_FLG_USE_USER_RW;
        }else   /* 'else' предотвращает лимит replyLen USB_NO_MSG для максимальной длины передачи. */
#endif
        if(sizeof(replyLen) < sizeof(rq->wLength.word)){ /* помогаем компилятору с оптимизацией */
            if(!rq->wLength.bytes[1] && replyLen > rq->wLength.bytes[0])    /* ограничить длину до max */
                replyLen = rq->wLength.bytes[0];
        }else{
            if(replyLen > rq->wLength.word)     														/* ограничить длину до max */
                replyLen = rq->wLength.word;
        }
        usbMsgLen = replyLen;
    }else{  /* usbRxToken должен быть USBPID_OUT, что означает фазу данных setup (control-out) */
#if USB_CFG_IMPLEMENT_FN_WRITE
        if(usbMsgFlags & USB_FLG_USE_USER_RW){
            uchar rval = usbFunctionWrite(data, len);
            if(rval == 0xff){   /* произошла ошибка */
                usbTxLen = USBPID_STALL;
            }else if(rval != 0){    /* Это последний пакет */
                usbMsgLen = 0;  /* ответ пакетом нулевой длины */
            }
        }
#endif
    }
}

/* ------------------------------------------------------------------------- */

/* Эта функция эквивалентна usbFunctionRead(), но она также вызывается для 
 *  данных, автоматически обрабатываемых драйвером (например, чтение дескриптора).
 */
static uchar usbDeviceRead(uchar *data, uchar len)
{
    if(len > 0){    /* не беспокоим приложение чтениями нулевого размера */
#if USB_CFG_IMPLEMENT_FN_READ
        if(usbMsgFlags & USB_FLG_USE_USER_RW){
            len = usbFunctionRead(data, len);
        }else
#endif
        {
            uchar i = len, *r = usbMsgPtr;
            if(usbMsgFlags & USB_FLG_MSGPTR_IS_ROM){    /* данные ROM */
                do{
                    uchar c = PRG_RDB(r);    /* назначаем переменную char для включения байтовых операций */
                    *data++ = c;
                    r++;
                }while(--i);
            }else{  /* данные RAM */
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

/* usbBuildTxBlock() вызывается, когда мы имеем данные для передачи, и когда
 *  опустошается буфер передачи подпрограммы прерывания.
 */
static inline void usbBuildTxBlock(void)
{
usbMsgLen_t wantLen;
uchar       len;

    wantLen = usbMsgLen;
    if(wantLen > 8)
        wantLen = 8;
    usbMsgLen -= wantLen;
    usbTxBuf[0] ^= USBPID_DATA0 ^ USBPID_DATA1; /* переключение DATA */
    len = usbDeviceRead(usbTxBuf + 1, wantLen);
    if(len <= 8){           /* допустимый пакет данных */
        usbCrc16Append(&usbTxBuf[1], len);
        len += 4;           /* длина включая байт sync */
        if(len < 12)        /* часть пакета идентифицирует конец сообщения */
            usbMsgLen = USB_NO_MSG;
    }else{
        len = USBPID_STALL;   /* остановка конечной точки */
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
/* Здесь мы должны проверить CRC16 -- но ACK все равно отсылается. Если Вам
 *  необходима проверка целостности данных в этом драйвере, проверяйте CRC в коде Вашего
 *  приложения и сообщайте об ошибках обратно хосту. Поскольку ACK отсылается по-любому,
 *  повторы при ошибках должны быть обработаны на уровне приложения.
 * unsigned crc = usbCrc16(buffer + 1, usbRxLen - 3);
 */
        usbProcessRx(usbRxBuf + USB_BUFSIZE + 1 - usbInputBufOffset, len);
#if USB_CFG_HAVE_FLOWCONTROL
        if(usbRxLen > 0)    /* если не деактивировано, доступен только mark */
            usbRxLen = 0;
#else
        usbRxLen = 0;       /* mark rx буфер как доступно */
#endif
    }
    if(usbTxLen & 0x10){    /* передача system idle */
        if(usbMsgLen != USB_NO_MSG){    /* данные для передачи в ожидании? */
            usbBuildTxBlock();
        }
    }
    for(i = 10; i > 0; i--){
        uchar usbLineStatus = USBIN & USBMASK;
        if(usbLineStatus != 0)  /* SE0 завершился */
            break;
    }
    if(i == 0){ /* состояние RESET, вызывается несколько раз во время сброса */
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
