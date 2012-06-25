/* Имя: oddebug.h
 * Проект: AVR library
 * Автор: Christian Starkjohann
 * Перевод: microsin.ru 
 * Дата создания: 2005-01-16
 * Табуляция: 4
 * Copyright: (c) 2005 by OBJECTIVE DEVELOPMENT Software GmbH
 * Лицензия: GNU GPL v2 (см. License.txt) или проприетарная (CommercialLicense.txt)
 * Ревизия: $Id: oddebug.h 275 2007-03-20 09:58:28Z cs $
 */

#ifndef __oddebug_h_included__
#define __oddebug_h_included__

/*
Основное описание:
Этот модуль реализует функцию для отладочных логов через последовательный интерфейс 
микроконтроллера AVR. Отладка может быть сконфигурирована оператором #define
DEBUG_LEVEL. Если этот макрос не задан или задан равным 0, все отладочные вызовы
не будут производить никаких действий. Если DEBUG_LEVEL задан в 1, появляются логи DBG1, 
но не DBG2. Если он задан 2, печатаются логи DBG1 и DBG2.

Лог отладки состоит из метки ('префикса', 'prefix') для индикации какой создан лог 
отладки и блок памяти для дампа в формате hex ('данные' 'data' и 'длина' 'len').
*/


#ifndef F_CPU
#   define  F_CPU   12000000    /* 12 МГц */
#endif

/* проверьте наши установки UART: */
#include "iarcompat.h"
#ifndef __IAR_SYSTEMS_ICC__
#   include <avr/io.h>
#endif

#ifndef uchar
#   define  uchar   unsigned char
#endif

#if DEBUG_LEVEL > 0 && !(defined TXEN || defined TXEN0) /* в микроконтроллере нет UART */
#   warning "Debugging disabled because device has no UART"
#   undef   DEBUG_LEVEL
#endif

#ifndef DEBUG_LEVEL
#   define  DEBUG_LEVEL 0
#endif

/* ------------------------------------------------------------------------- */

#if DEBUG_LEVEL > 0
#   define  DBG1(prefix, data, len) odDebug(prefix, data, len)
#else
#   define  DBG1(prefix, data, len)
#endif

#if DEBUG_LEVEL > 1
#   define  DBG2(prefix, data, len) odDebug(prefix, data, len)
#else
#   define  DBG2(prefix, data, len)
#endif

/* ------------------------------------------------------------------------- */

#if DEBUG_LEVEL > 0
extern void odDebug(uchar prefix, uchar *data, uchar len);

/* Попытка найти регистры управления; ATMEL могла их переименовать */

#if defined UBRR
#   define  ODDBG_UBRR  UBRR
#elif defined UBRRL
#   define  ODDBG_UBRR  UBRRL
#elif defined UBRR0
#   define  ODDBG_UBRR  UBRR0
#elif defined UBRR0L
#   define  ODDBG_UBRR  UBRR0L
#endif

#if defined UCR
#   define  ODDBG_UCR   UCR
#elif defined UCSRB
#   define  ODDBG_UCR   UCSRB
#elif defined UCSR0B
#   define  ODDBG_UCR   UCSR0B
#endif

#if defined TXEN
#   define  ODDBG_TXEN  TXEN
#else
#   define  ODDBG_TXEN  TXEN0
#endif

#if defined USR
#   define  ODDBG_USR   USR
#elif defined UCSRA
#   define  ODDBG_USR   UCSRA
#elif defined UCSR0A
#   define  ODDBG_USR   UCSR0A
#endif

#if defined UDRE
#   define  ODDBG_UDRE  UDRE
#else
#   define  ODDBG_UDRE  UDRE0
#endif

#if defined UDR
#   define  ODDBG_UDR   UDR
#elif defined UDR0
#   define  ODDBG_UDR   UDR0
#endif

static inline void  odDebugInit(void)
{
    ODDBG_UCR |= (1<<ODDBG_TXEN);
    ODDBG_UBRR = F_CPU / (19200 * 16L) - 1;
}
#else
#   define odDebugInit()
#endif

/* ------------------------------------------------------------------------- */

#endif /* __oddebug_h_included__ */
