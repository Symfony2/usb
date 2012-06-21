/* ���: oddebug.h
 * ������: AVR library
 * �����: Christian Starkjohann
 * �������: microsin.ru 
 * ���� ��������: 2005-01-16
 * ���������: 4
 * Copyright: (c) 2005 by OBJECTIVE DEVELOPMENT Software GmbH
 * ��������: GNU GPL v2 (��. License.txt) ��� ������������� (CommercialLicense.txt)
 * �������: $Id: oddebug.h 275 2007-03-20 09:58:28Z cs $
 */

#ifndef __oddebug_h_included__
#define __oddebug_h_included__

/*
�������� ��������:
���� ������ ��������� ������� ��� ���������� ����� ����� ���������������� ��������� 
���������������� AVR. ������� ����� ���� ���������������� ���������� #define
DEBUG_LEVEL. ���� ���� ������ �� ����� ��� ����� ������ 0, ��� ���������� ������
�� ����� ����������� ������� ��������. ���� DEBUG_LEVEL ����� � 1, ���������� ���� DBG1, 
�� �� DBG2. ���� �� ����� 2, ���������� ���� DBG1 � DBG2.

��� ������� ������� �� ����� ('��������', 'prefix') ��� ��������� ����� ������ ��� 
������� � ���� ������ ��� ����� � ������� hex ('������' 'data' � '�����' 'len').
*/


#ifndef F_CPU
#   define  F_CPU   12000000    /* 12 ��� */
#endif

/* ��������� ���� ��������� UART: */
#include "iarcompat.h"
#ifndef __IAR_SYSTEMS_ICC__
#   include <avr/io.h>
#endif

#ifndef uchar
#   define  uchar   unsigned char
#endif

#if DEBUG_LEVEL > 0 && !(defined TXEN || defined TXEN0) /* � ���������������� ��� UART */
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

/* ������� ����� �������� ����������; ATMEL ����� �� ������������� */

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
