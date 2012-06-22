/* ���: iarcompat.h
 * ������: AVR USB driver
 * �����: Christian Starkjohann
 * �������: microsin.ru 
 * ���� ��������: 2006-03-01
 * ���������: 4
 * Copyright: (c) 2006 by OBJECTIVE DEVELOPMENT Software GmbH
 * ��������: GNU GPL v2 (��. License.txt) ��� ������������� (CommercialLicense.txt)
 * �������: $Id: iarcompat.h 533 2008-02-28 15:35:25Z cs $
 */

/*
�������� ��������:
���� ������������ ���� ����������, ����� �� ����������� � IAR C-������������ � �����������.
���� ������ ������ ��� ������������� ����� gcc � IAR-cc.

���������� ����� �������� �� ������ � ������������ �� IAR!
*/

#ifndef __iarcompat_h_INCLUDED__
#define __iarcompat_h_INCLUDED__

#if defined __IAR_SYSTEMS_ICC__ || defined __IAR_SYSTEMS_ASM__

/* ���������� ������� ����� */
#ifndef ENABLE_BIT_DEFINITIONS
#   define ENABLE_BIT_DEFINITIONS	1
#endif

/* ��������� ������������ ������ IAR */
#include <ioavr.h>
#ifndef __IAR_SYSTEMS_ASM__
#   include <inavr.h>
#endif

#define __attribute__(arg)

#ifdef __IAR_SYSTEMS_ASM__
#   define __ASSEMBLER__
#endif

#ifdef __HAS_ELPM__
#   define PROGMEM __farflash
#else
#   define PROGMEM __flash
#endif

#define PRG_RDB(addr)   (*(PROGMEM char *)(addr))

/* ��������� ����������� �� ����� ��� ��������, ������ ����� ������,
 * ���� �� ���������� �� IAR ������, ���������� �� gcc.
 */
#define cli()       __disable_interrupt()
#define sei()       __enable_interrupt()
#define wdt_reset() __watchdog_reset()

/* � ����������� �� ������������� ���������������� (MCU) �� ������ ��������
 *  �������� ��-�� ����, ��� ������ usbdrv.h ��� ������� ���� MCU ����������. 
 *  ��������� IAR �� ���������� ��������� #define ��� ��������� MCU, �� �� �����
 *  ��������� ������� ������������� �������� � ������� #ifdef. ���� �������� 
 *  ��������������� �� �����������, �������� ����������� ��� ������� �������
 *  USB_INTR_* � ��� usbconfig.h. ��. ����������� � usbconfig-prototype.h � usbdrv.h.
 */

#endif  /* defined __IAR_SYSTEMS_ICC__ || defined __IAR_SYSTEMS_ASM__ */
#endif  /* __iarcompat_h_INCLUDED__ */
