/* Имя: oddebug.c
 * Проект: библиотека AVR
 * Автор: Christian Starkjohann
 * Перевод: microsin.ru 
 * Дата создания: 2005-01-16
 * Табуляция: 4
 * Copyright: (c) 2005 by OBJECTIVE DEVELOPMENT Software GmbH
 * Лицензия: GNU GPL v2 (см. License.txt) или проприетарная (CommercialLicense.txt)
 * Ревизия: $Id: oddebug.c 275 2007-03-20 09:58:28Z cs $
 */

#include "oddebug.h"

#if DEBUG_LEVEL > 0

#warning "Never compile production devices with debugging enabled"

static void uartPutc(char c)
{
    while(!(ODDBG_USR & (1 << ODDBG_UDRE)));    /* ожидание очистки регистра данных */
    ODDBG_UDR = c;
}

static uchar    hexAscii(uchar h)
{
    h &= 0xf;
    if(h >= 10)
        h += 'a' - (uchar)10 - '0';
    h += '0';
    return h;
}

static void printHex(uchar c)
{
    uartPutc(hexAscii(c >> 4));
    uartPutc(hexAscii(c));
}

void    odDebug(uchar prefix, uchar *data, uchar len)
{
    printHex(prefix);
    uartPutc(':');
    while(len--){
        uartPutc(' ');
        printHex(*data++);
    }
    uartPutc('\r');
    uartPutc('\n');
}

#endif
