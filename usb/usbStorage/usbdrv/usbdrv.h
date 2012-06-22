/* ���: usbdrv.h
 * ������: ������� AVR USB
 * �����: Christian Starkjohann
 * �������: microsin.ru  
 * ���� ��������: 2004-12-29
 * ���������: 4
 * Copyright: (c) 2005 by OBJECTIVE DEVELOPMENT Software GmbH
 * ��������: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
 * �������: $Id: usbdrv.h 607 2008-05-13 15:57:28Z cs $
 */

#ifndef __usbdrv_h_included__
#define __usbdrv_h_included__
#include "usbconfig.h"
#include "iarcompat.h"

/*
���������� � ������:
====================
������� USB D+ � D- ������ ���� ������������ �� ���� � ��� �� I/O ���� (A, B, C ��� D). 
�� �����������, ����� ������ D+ ������� ���������� (������ ����� - ������������ INT0 
��� D+), ������ ��������, ����� ���������� ����������� �� �� D+, � �� D-. ���� 
������������ D-, ���������� ����� ����������� ������ ��� ��� ��������� ������� SOF
(Start-Of-Frame). D- ������� ����������� (pull-up) �������� 1.5k, ������������ � +3.5V 
(� ���������� ������ ���� �������� �� 3.5V) ��� ������������� USB ���������� ��� 
low-speed. �������� pull-down ��� pull-up 1M ������ ���� SHOULD ����������� ����� D+ 
� +3.5V ��� �������������� �����, ������ ����������� USB ������. ���� �� �����������
����� ������ (��-������ - ������������) ��� ����������� ���������� �� �������� D+ � D-, 
�� ������ ������������ �������� pull-down, � �� pull-up. �� ���������� D+ ��� ��������
����������, �� D-, ������ ��� �� �� �������� ���������� �� ��������� keep-alive 
� RESET. ���� �� ������ ������������ ������� keep-alive � USB_COUNT_SOF, �� ������
������������ D- ��� �������� ����������.

��� ����� ����� ����������, 1.5k pull-up �������� �� D- ������ ������� �����������,
����� ����������� ����������� ���������� ���������� �� ���� USB. ��. ���������
usbDeviceConnect() � usbDeviceDisconnect() ����� � ���� �����.

����������, ��������� �������� � ����� usbconfig.h � ������������ ������ ������!

��������������� � ����� USB-���������� ������ ������������� �� ������ 12, 15 ��� 16 ���
(��� ������������� ������) ��� 16.5 ��� (������������� ����������� RC-����������, 
�������� ������� +/- 1%). ������ ��. � usbconfig-prototype.h.


�����������:
============
������������ � ������� � ������������:
������� ������������, ��� ��� �������� ������ ������ �����������. �� ������ ��������
������ � PID, �� �� ��������� ������ ������� �����, SE0 � �������� �����,
CRC ������ (5 ���) � CRC ������ (16 ���). �������� CRC �� ����� ���� ��������� ��-��
����������� �� �������: �� ������ ������ �������� ������ ������ 7-������� ���������
�������. ������� ������ � ����������� ����������� SE0 �� ������������ ������ �����������
� ������ ��������� �������, �� ����������� �������������� CPU AVR �� ��������� ���. 
������� �� ��������� ������������ Data0/Data1, �� ����������, � ������� �������� �������,
����� ����������� ����� ��������.

������� ��������������:
��������� ������������ ����� ������������������� ���������, ��������������� 
� ������������� ������� ����������. ������� ���������� ������ ���� ����� �
������ � �������� �������� ���������������� ����� I/O. ������ ��������� �����
����������� ��������������� ������������ USB, �� ������������ AVR ����������� 
����������. ����� ������������ ������� ����������� ��������������� ��������� 
�� ������� ��������� ��� ����������� SE0.

���������� �������� �����:
������� ������������ ��������� �������� �����:

- Endpoint 0, �� ��������� ����������� �������� ����� (default control endpoint).
- ����� ���������� interrupt- ��� bulk-out �������� �����. ������ ����������
  � usbFunctionWriteOut() � USB_CFG_IMPLEMENT_FN_WRITEOUT ������ ���� �����
  � 1 ��� ������������� ���� �����������. ����� �������� ����� ����� ���� 
  ������ � ���������� ���������� 'usbRxToken'.
- �� ��������� ���� interrupt- ��� bulk-in �������� �����. ��� �������� �����
  ������������ ��� interrupt- ��� bulk-in �������, ������� �� ��������������
  ������ �������� ������. �� ������ ������ USB_CFG_HAVE_INTRIN_ENDPOINT � �������
  ������������� ���� ����������� � ������� usbSetInterrupt() ��� ��������
  ������ interrupt/bulk.
- ���� ���������� interrupt- ��� bulk-in �������� �����. ��� ���� endpoint 3 �
  ���������� ������� ����� ��������, �� ����� ���������������� ����� ����� 
  �������� �����. �� ������ ������ USB_CFG_HAVE_INTRIN_ENDPOINT3 ��� ���������
  ���� ����������� � ������� usbSetInterrupt3() ��� �������� ������ interrupt/bulk. 
  ����� �������� ����� ����������� � USB_CFG_EP3_NUMBER.

���������� ������� �� ��������, ��� �������� USB ��������� �������� ����� bulk 
��� ��������������� ���������! ������ ������������ ������� ������ ��� ����� 
��������� ������������ ��, �� AVR ������ 90% ������� CPU � ������� ����������
USB ��� ������ bulk.

������������ �������� �������� �������:
�������� ������� ��� ����������� ������� (control in) � �������� (out) �������
����� ��������� �� 254 ����. ����� ������� ������ �������� �������� ��������
������� (out transfers), �� ������ ����������� 'usbFunctionWrite()'.

����������� ���� � ������ USB Suspend Mode:
�������� USB ������������ ����������������� ��������� 500uA, ����� ���� 
��������� � ����� suspend mode. ��� �� �������� ��� ���������, ������� 
������������ ��������������, �� ���������� ��������� ���� (�������� �� �� 
���� USB). ����������, ���������� �� ���� USB, ����� ��������� ��� ����������
������ � ��� ������, ���� CPU (� ����� ������ ��������������� AVR) ���������
� ����� ��� (sleep mode). �������������� ������� �� ������������ ���������
������ suspend. ������ ���������� ����� ����������� ���������� ����������
� ����������� �� ���. ���� ��������� ���������� ��������� SE0 ��� �����������
���� � �������� ���������. ��� ��������� SE0 ����� ��������������� ��� 
������������� ������� D- � �������� ��������� ����������. ������� USB_COUNT_SOF 
� 1 � ����������� ���������� ���������� usbSofCount ��� �������� ���������� ����.

������ ��� ������� USB:
������� ����� ���� ���������� ��� ����������� � ������� USB, ���� D- ��������
��� 1. ����� �������� ��������� ����������, �� ����������� �� ������ D+ 
(����������) ���������� �������� �������� ������������� (�������� 1M) ��� 
pull-down ("������" ��������, ������������ ����� ������ � D+) ��� pull-up 
("�������" ��������, ������������ ����� ����� ������� +3.3 � � D+). 
���� ������������ ����� ������ (������������), ����������� pull-down. 
���� D- ���������� ���������� � 0, ������� ����� ������������� ������������ 
����������.

����� �������� ��������� ���������� (Interrupt latency):
���������� ������ ��������������, ��� USB ���������� �� ��������� ��� ����� ���
25 ������ (��� ����� ��� 12 ���, ��� ������� �������� ������� ��������� 
������� ���������� ������, ��� � ���������� latency). ��� �������������, ���
��� ����������� ���������� ������ ���� ���� ��������������� ��� "INTERRUPT"
������ "SIGNAL" (��. "avr/signal.h") ���� �������� �� ���������� � ������ 
����������� "sei".

������������ ������������ ���������� / ����������� ������ CPU:
������� ������������ ��� ���������� USB ������ ����������� ����������. �����
�� ����������� ���������� �� ����������, ���� �� ����� ������� ��� USB-���������
������� � ���������� �������������. ��� ����� ������� �������� �� 1200 ������ �� 
������� 12 MHz (= 100us), ���� ���� �������� ����������� ���������. ������� 
����� ���������� ����� CPU ��� ���� ��������� USB, ���� ���� ��� ���������� 
� ������� (low-speed) ���������� �� ��� �� ����.

*/

/* ------------------------------------------------------------------------- */
/* --------------------------- ��������� ������ ---------------------------- */
/* ------------------------------------------------------------------------- */

#define USBDRV_VERSION  20080513
/* ����� �������� ���������� ������������� ������ ��������. ��� ���������� �����,
 *  ������������ �� ���� ������ �������� � ����� YYYYMMDD. ���� ��������� ����������
 *  �������� ��������, �� ������ ������������ ��� ���������, ����� �������� ������. 
 *  ���� �� �������, �� ���� ������ �������� ������ ��� 2006-01-25.
 */


#ifndef USB_PUBLIC
#define USB_PUBLIC
#endif
/* USB_PUBLIC ������������ ��� ������� ���������� ��� ���� �������, ��������������
 *  ��������� USB. �� ��������� ������� �� ����� (��. ����). �� ������ ������
 *  ������� static ���� � usbconfig.h ��� �� ��������� ������, ���� �� ���������
 *  usbdrv.c ������ ��� ��������. ��������� ������ C �������� �������� � ��� ���
 *  �������� ��������� ���� ������ flash (������ ��������).
 */

#ifndef __ASSEMBLER__
#ifndef uchar
#define uchar   unsigned char
#endif
#ifndef schar
#define schar   signed char
#endif
/* �������� 8 ��� ����� ����� */

#if USB_CFG_LONG_TRANSFERS  /* ���� ��������� ��� �������� ������, ��� 254 ����� */
#   define usbMsgLen_t unsigned
#else
#   define usbMsgLen_t uchar
#endif
/* usbMsgLen_t ���������� ��� ������, ������������ ��� ���� ��������. �� ���������
 *  ������ uchar, ��� ��������� �������� �������� 254 ���� (����� 255 ���������������
 *  ��� USB_NO_MSG �����). ���� � ����� usbconfig.h ������ USB_CFG_LONG_TRANSFERS � 1,
 *  ����� �������������� ��� ������ 16 ���, ��� ��������� �� 16384 ���� (������� 
 *  ������������ ��� ������ � ������������ �����������).
 */
#define USB_NO_MSG  ((usbMsgLen_t)-1)   /* ���������, ������� �������� "��� ���������" */

struct usbRequest;  /* ��������������� ���������� */

USB_PUBLIC void usbInit(void);
/* ��� ������� ������ ���� ������� ����� ����������� ���������� � ������� � �������� 
 *  ���� main.
 */
USB_PUBLIC void usbPoll(void);
/* ��� ������� ������ ���� ������� ����� ���������� ��������� ������ �������� ����� main.
 *  ������������ �������� ����� �������� ������ ���� ��������� ������ 50 �� (������� USB ���
 *  �������� ��������� Setup). ����� ���������� �� ����� ����������.
 * ���������� ������� �� ��������, ��� ���������� ����� ����� UART �������� ~ 0.5 �� �� ����
 *  ��� �������� 19200 bps.
 */
extern uchar *usbMsgPtr;
/* ��� ���������� ����� �������������� ��� �������� ������������ ������ �������� 
 *  �� ���������� usbFunctionWrite(). ��� ����� ������������ ��������� ���������
 *  ��� ����������� ����������� ��������.
 */
USB_PUBLIC usbMsgLen_t usbFunctionSetup(uchar data[8]);
/* ��� ������� ����������, ����� ������� ��������� ���������� SETUP �� �����,
 *  �������� �� �������� ������� ��������������� (�� ��������: ������� ������
 *  � �������). ��� ����������� �������� �������� � ���������� SETUP, ��� ����
 *  �������� ��������� ���������� (�����������) ������� ������. ������ SETUP 
 *  �������� � ��������� 'data', ������� ����� (� ������) ���� ������������ 
 *  � 'usbRequest_t *' ��� ������� �������������� ��� ������������ �������
 *  � ����������.
 *
 * ���� SETUP ���������� �������� control-in, �� ������ ���������� ��� ��������
 *  ����������� ������. ���� ��� ���� ��� �������� ���� ������:
 *  (1) ���������� ���������� ��������� 'usbMsgPtr' �� ������ ����� ������ 
 *  static RAM � ���������� ����� ������ � 'usbFunctionSetup()'. ������� 
 *  ���������� �������. ��� (2) ���������� USB_NO_MSG � 'usbFunctionSetup()'. 
 *  ������� ����� �������� 'usbFunctionRead()', ����� ����������� ������. ���
 *  ������������ ��. ������������ usbFunctionRead().
 *
 * ���� SETUP ���������� �������� control-out, ���� ������ ���� ���� ��� ������
 *  ������ �� ����� - ����� ����� 'usbFunctionWrite()'. ���� �� ����������� ��� 
 *  �������, �� ������ ���������� USB_NO_MSG � 'usbFunctionSetup()', �����  
 *  �������� ������������� ������������� 'usbFunctionWrite()'. ��� ������� 
 *  ���������� ��. ������������ �� ���� �������. ���� �� ������ ������ ������������
 *  ������, ������������ ������, ������� 0 'usbFunctionSetup()'.
 *
 * ������� �� ��������, ��� ������ ������� usbFunctionRead() � usbFunctionWrite()
 *  �������� ������ ���� ��� ��������� ������������� � usbconfig.h.
 */
USB_PUBLIC usbMsgLen_t usbFunctionDescriptor(struct usbRequest *rq);
/* ��� ����� ����������� ��� ������� ������ ���� �� �������������� ����������� USB
 *  �� ����� ���������� - runtime (����������� ��� ���������). ��� ����� �������
 *  usbFunctionSetup() ����, �� ��� ���������� ������ ��� ������� ������ �����������
 *  USB data. ��������� ��. ������������ usbFunctionSetup() ����.
 */
#if USB_CFG_HAVE_INTRIN_ENDPOINT
USB_PUBLIC void usbSetInterrupt(uchar *data, uchar len);
/* ��� ������� ������������� ���������, ������� ����� ���������� ��� ��������� 
 *  �������� IN �� ����������. ��������� ���������� �� ���������� ����� � �� ������
 *  ��������� �� ����� 8 ����. ��������� ����� ���� ������ � 0 ���� ������ ��� 
 *  ��������� ������� ���������� ��� �����.
 * ���� ��� ����� �������� ������� ���������� ����, ����������� ����������� ������
 *  ����� ����������.
 */
#define usbInterruptIsReady()   (usbTxLen1 & 0x10)
/* ���� ������ ����������, ���� �� ���� ������� ��������� ��������� ����������. 
 *  ���� �� ���������� ����� ��������� ���������� ����� ���, ��� ������ ����
 *  ����������, �� ��� ��������������� ��������� ����� ��������.
 */
#if USB_CFG_HAVE_INTRIN_ENDPOINT3
USB_PUBLIC void usbSetInterrupt3(uchar *data, uchar len);
#define usbInterruptIsReady3()   (usbTxLen3 & 0x10)
/* �� �� �����, ��� �����, �� ��� �������� ����� 3 */
#endif
#endif /* USB_CFG_HAVE_INTRIN_ENDPOINT */
#if USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH    /* ���������� ��������� ��� �������� ������������� */
#define usbHidReportDescriptor  usbDescriptorHidReport
/* ������ ���� ������������ ���: PROGMEM char usbHidReportDescriptor[]; */
/* ���� �� ���������� ���������� HID, ��� ����� ������������ ���������� �������.
 * HID ������ ���������� ����� ��������� ����������� ���������. ���� �� ���������,
 *  ��� �������������� ������ �����������, �� ����������� ������������
 *  HID Descriptor Tool �� usb.org, ��. http://www.usb.org/developers/hidpage/.
 *  � ������ ������ ��� ����� ������ � �������� �������.
 */
#endif  /* USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH */
#if USB_CFG_IMPLEMENT_FN_WRITE
USB_PUBLIC uchar usbFunctionWrite(uchar *data, uchar len);
/* ��� ������� ���������� ��������� ��� ����������� ����������� ������� 
 *  (control transfer's) ������ �������� �������� (control-out). ��� ����������
 *  � ������ �� 8 ����. ����� ����������, ��������������� � ������� ����������� 
 *  ��������, ����� ���� �������� �� �������� 'length' ������ setup. ���� 
 *  �� ����� ��������� ��������� ������, ������������ 0xff (== -1). � ���� 
 *  ������ ������� ����� �������� ��� ���� �������� ������� STALL. ���� ��
 *  ������� ������� ��� �������� ��������, ������������ 1. ���� �� ��������
 *  ������ ������, ������� 0. ���� �� �� ������, ����� �� ���� �������� ���
 *  ������ (�� ������ ����� ����� ���������� ��������������� ������� 
 *  usbFunctionSetup()!), ������������ 1.
 * ���������: ���� �� ������� 0xff ��� STALL, 'usbFunctionWrite()' ����� ��� 
 *  ����� ���������� ��� ���������� ������. �� ������ ���������� ���������� 0xff 
 *  ��� STALL � ���� �������.
 * ��� ����, ����� �������� ������� usbFunctionWrite(), ������� 
 *  USB_CFG_IMPLEMENT_FN_WRITE �� 1 � usbconfig.h � ���������� 0xff � usbFunctionSetup().
 */
#endif /* USB_CFG_IMPLEMENT_FN_WRITE */
#if USB_CFG_IMPLEMENT_FN_READ
USB_PUBLIC uchar usbFunctionRead(uchar *data, uchar len);
/* ��� ������� ���������� ��������� ��� ������� � ���������� ����������� �������
 *  ������ �������� �������� (control-in). ���������� ������� �� 8 ���� ������. 
 *  �� ������ ����������� ������ � �����, ��������� ���������� 'data' �
 *  ���������� ������������� ������������� ���������� ����. ���� �� ������� return 
 *  ����� �������, ��� �������������, �������� control-in ����� ��������. ���� 
 *  �� ���������� 0xff, ������� ������� �������� � ������� STALL.
 * ��� ����, ����� ���������� usbFunctionRead(), ������� USB_CFG_IMPLEMENT_FN_READ
 *  �� 1 � usbconfig.h � ������� 0xff � usbFunctionSetup().
 */
#endif /* USB_CFG_IMPLEMENT_FN_READ */
#if USB_CFG_IMPLEMENT_FN_WRITEOUT
USB_PUBLIC void usbFunctionWriteOut(uchar *data, uchar len);
/* ��� ������� ���������� ��������� ����� ������ ����������� �������� ������
 *  interrupt ��� bulk-out. ����� �������� ����� ����� ����� � ���������� 
 *  ���������� usbRxToken. �� ������ ������ USB_CFG_IMPLEMENT_FN_WRITEOUT �� 1 
 *  � usbconfig.h, ����� �������� ������ ���� �������.
 */
#endif /* USB_CFG_IMPLEMENT_FN_WRITEOUT */
#ifdef USB_CFG_PULLUP_IOPORTNAME
#define usbDeviceConnect()      ((USB_PULLUP_DDR |= (1<<USB_CFG_PULLUP_BIT)), \
                                  (USB_PULLUP_OUT |= (1<<USB_CFG_PULLUP_BIT)))
#define usbDeviceDisconnect()   ((USB_PULLUP_DDR &= ~(1<<USB_CFG_PULLUP_BIT)), \
                                  (USB_PULLUP_OUT &= ~(1<<USB_CFG_PULLUP_BIT)))
#else /* USB_CFG_PULLUP_IOPORTNAME */
#define usbDeviceConnect()      (USBDDR &= ~(1<<USBMINUS))
#define usbDeviceDisconnect()   (USBDDR |= (1<<USBMINUS))
#endif /* USB_CFG_PULLUP_IOPORTNAME */
/* ������ usbDeviceConnect() � usbDeviceDisconnect() (��������������� ����������
 *  �������) ��������� � ��������� ���������� � ����� USB �����. ���� ��������� 
 *  USB_CFG_PULLUP_IOPORT � USB_CFG_PULLUP_BIT ������ � usbconfig.h, ������������
 *  ����������� � �������� ��������� pull-up �� D-, ����� ������������ ����������
 *  ��� ������ ������������ D- � GND (D- ���������� ��������������� ��� ����� 0).
 *  ��� �� ������������� ������������, �� ��������.
 * ���������� ������� �� ��������, ��� ���������� USB ������ ���� ���������,
 *  ����� ���������� ��������� � ����������� ���������, ��� ���������� ����������
 *  ��������! �� ������ ���� ��������� ���������� USB ����������:
 *     USB_INTR_ENABLE &= ~(1 << USB_INTR_ENABLE_BIT)
 *  ��� ��������� cli() ��� ����������� ������� ���� ����������.
 */
extern unsigned usbCrc16(unsigned data, uchar len);
#define usbCrc16(data, len) usbCrc16((unsigned)(data), len)
/* ��� ������� ��������� �������� ���������� CRC ������, ������������ � �������
 *  ������ USB. �������� ������������ ��� ����������� ����� (raw) ������������
 *  �������. �� ������ �������� ������������ ��� ������� ��� ����������� ���� 
 *  ������ ��� �������� �������� ������. �� ������������� ���������� 16 ��� 
 *  ���������� � ������� ��� ������������� � tiny ������� ������ IAR.
 */
extern unsigned usbCrc16Append(unsigned data, uchar len);
#define usbCrc16Append(data, len)    usbCrc16Append((unsigned)(data), len)
/* ��� ������� ��������� ������������ usbCrc16() ����, ������� ������ � ���, 
 *  ��� ��� ��������� 2 ����� CRC (������ ������� ����) � ����� 'data' �����
 * ������ 'len' ����.
 */
#if USB_CFG_HAVE_MEASURE_FRAME_LENGTH
extern unsigned usbMeasureFrameLength(void);
/* ��� ������� ������ ���� ������� ���������� ����� ����� ������ USB � ���
 *  �������� 1/7 ���������� ������ CPU �� ����� ������ USB ����� ���� ��� �����
 *  ���������������� ����. ������� �������: ������������ �������� 
 *  = 1499 * (F_CPU / 10.5 MHz).
 * �� ����� ����� ��������� ��������, �� ������ ��������� ��� ����������,
 *  ��������� cli() ����� ������� ���� �������. ������� ����� �������������� 
 *  ��� ���������� RC-���������� AVR.
 */
#endif
extern uchar usbConfiguration;
/* ��� ���������� �������� ����� ������� ������������, ������������� ������. 
 *  ������� ��������� ��������� � ����� ���� ���������� � ��������� USB
 *  SET_CONFIGURATION � GET_CONFIGURATION, �� �� ���������� � �����.
 * �� ������ �������� "������������������" ��������� ����������� (LED) 
 *  �� ���������� ��� �������� ���� �����, ������������ ������� ��������,
 *  ������ ���� ���������� ����������������.
 */
#if USB_COUNT_SOF
extern volatile uchar   usbSofCount;
/* ��� ������������ ���������������� ������ ������� SOF (Start-Of-Frame, ������
 *  ������). ��� �������� ������ � ��� ������, ���� ����� USB_COUNT_SOF �����
 *  � �������� != 0.
 */
#endif

#define USB_STRING_DESCRIPTOR_HEADER(stringLength) ((2*(stringLength)+2) | (3<<8))
/* ���� ����� ������ ��������� ����������� ��� ���������� �����������,
 *  �� ��������� ����� ������. ��. ��� ������� ������������� usbdrv.c.
 */
#if USB_CFG_HAVE_FLOWCONTROL
extern volatile schar   usbRxLen;
#define usbDisableAllRequests()     usbRxLen = -1
/* ������ ���� ������� �� usbFunctionWrite(). ���� ������ ��������� ��� �������
 *  ������ �� ���������� USB. �� ������� �� ����� ���������� ������ NAK, ���� 
 *  ��������� ����� ������.
 */
#define usbEnableAllRequests()      usbRxLen = 0
/* ����� ���� ������� ������ ���� ������� ���������. ���� ������ ��������� ����
 *  �� ���������� USB ����� ����, ��� �� ��� �������� ������� usbDisableAllRequests().
 */
#define usbAllRequestsAreDisabled() (usbRxLen < 0)
/* ���� ������ ���������� TRUE, ���� ������� ���� ���������. �� ����� ������������
 *  ��� ����, ����� ��������� � ���, ��� usbEnableAllRequests() �� ����������, 
 *  ����� ������� ���������.
 */
#endif

#define USB_SET_DATATOKEN1(token)   usbTxBuf1[0] = token
#define USB_SET_DATATOKEN3(token)   usbTxBuf3[0] = token
/* ��� ��� ������� ����� �������������� ���������� ���������� ��� ������ ������������
 *  ������ ��� �������� ����� interrupt-in � �������� 1 � 3. ��������� ����� 
 *  ������������� ����� ��������� ������, �� ������ ���������� ��������������� 
 *  �������� ������, ������� ������ ������ ������.
 */

#endif  /* __ASSEMBLER__ */


/* ------------------------------------------------------------------------- */
/* -------------------- ����������� ������� ����������� -------------------- */
/* ------------------------------------------------------------------------- */
/* ��� ��������� ��������. ��. usbconfig-prototype.h ��� ������������ 
 *  � ��������� ������� ����������� ������������ USB. ���� �� ������ �� ��������,
 *  ����� �������������� ����������� �� ���������.
 */
#define USB_PROP_IS_DYNAMIC     (1 << 14)
/* ���� ��� �������� ����������� ��� �����������, usbFunctionDescriptor() �����
 *  �������������� ��� ��������� �������� �����������.
 */
#define USB_PROP_IS_RAM         (1 << 15)
/* ���� ��� �������� ����������� ��� �����������, ������ �������� �� ������ RAM
 *  ������ Flash. �������� ������������ ����� �������� ��� ��������������
 *  ������� ������������.
 */
#define USB_PROP_LENGTH(len)    ((len) & 0x3fff)
/* ���� ������������ ����������� ������� ����������, ��� ����� ����� ����������� 
 *  � ������.
 */

/* ��� �����������, ������� ����� ����� ��������: */
#ifndef USB_CFG_DESCR_PROPS_DEVICE
#define USB_CFG_DESCR_PROPS_DEVICE                  0
#endif
#ifndef USB_CFG_DESCR_PROPS_CONFIGURATION
#define USB_CFG_DESCR_PROPS_CONFIGURATION           0
#endif
#ifndef USB_CFG_DESCR_PROPS_STRINGS
#define USB_CFG_DESCR_PROPS_STRINGS                 0
#endif
#ifndef USB_CFG_DESCR_PROPS_STRING_0
#define USB_CFG_DESCR_PROPS_STRING_0                0
#endif
#ifndef USB_CFG_DESCR_PROPS_STRING_VENDOR
#define USB_CFG_DESCR_PROPS_STRING_VENDOR           0
#endif
#ifndef USB_CFG_DESCR_PROPS_STRING_PRODUCT
#define USB_CFG_DESCR_PROPS_STRING_PRODUCT          0
#endif
#ifndef USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER
#define USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER    0
#endif
#ifndef USB_CFG_DESCR_PROPS_HID
#define USB_CFG_DESCR_PROPS_HID                     0
#endif
#if !(USB_CFG_DESCR_PROPS_HID_REPORT)
#   undef USB_CFG_DESCR_PROPS_HID_REPORT
#   if USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH /* ��������� ��������� ��� �������� ������������� */
#       define USB_CFG_DESCR_PROPS_HID_REPORT       USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH
#   else
#       define USB_CFG_DESCR_PROPS_HID_REPORT       0
#   endif
#endif
#ifndef USB_CFG_DESCR_PROPS_UNKNOWN
#define USB_CFG_DESCR_PROPS_UNKNOWN                 0
#endif

/* ------------------ ��������������� ���������� ������������ ------------------- */
/* ���� �� ����������� ������� ����������� �����������, ��� ������ ���� ��������� 
 *  � ���������� ��������, ��� ��������� ����:
 */
#ifndef __ASSEMBLER__
extern
#if !(USB_CFG_DESCR_PROPS_DEVICE & USB_PROP_IS_RAM)
PROGMEM
#endif
char usbDescriptorDevice[];

extern
#if !(USB_CFG_DESCR_PROPS_CONFIGURATION & USB_PROP_IS_RAM)
PROGMEM
#endif
char usbDescriptorConfiguration[];

extern
#if !(USB_CFG_DESCR_PROPS_HID_REPORT & USB_PROP_IS_RAM)
PROGMEM
#endif
char usbDescriptorHidReport[];

extern
#if !(USB_CFG_DESCR_PROPS_STRING_0 & USB_PROP_IS_RAM)
PROGMEM
#endif
char usbDescriptorString0[];

extern
#if !(USB_CFG_DESCR_PROPS_STRING_VENDOR & USB_PROP_IS_RAM)
PROGMEM
#endif
int usbDescriptorStringVendor[];

extern
#if !(USB_CFG_DESCR_PROPS_STRING_PRODUCT & USB_PROP_IS_RAM)
PROGMEM
#endif
int usbDescriptorStringDevice[];

extern
#if !(USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER & USB_PROP_IS_RAM)
PROGMEM
#endif
int usbDescriptorStringSerialNumber[];

#endif /* __ASSEMBLER__ */

/* ------------------------------------------------------------------------- */
/* ----------------------- ������� ������ ���������� ----------------------- */
/* ------------------------------------------------------------------------- */

#define USB_CONCAT(a, b)            a ## b
#define USB_CONCAT_EXPANDED(a, b)   USB_CONCAT(a, b)

#define USB_OUTPORT(name)           USB_CONCAT(PORT, name)
#define USB_INPORT(name)            USB_CONCAT(PIN, name)
#define USB_DDRPORT(name)           USB_CONCAT(DDR, name)
/* �����, �������� ������ ���� ��������� ��� ��������� ������, �������� ��������.
 */

/* ------------------------------------------------------------------------- */
/* ------------------------- ����������� �������� -------------------------- */
/* ------------------------------------------------------------------------- */

#if !defined __ASSEMBLER__ && (!defined USB_CFG_VENDOR_ID || !defined USB_CFG_DEVICE_ID)
#warning "You should define USB_CFG_VENDOR_ID and USB_CFG_DEVICE_ID in usbconfig.h"
/* ���� ������������ �� ����� �������������� ID, �� ���������� �� ��������� 
 *  ��������� obdev ID. ������ ��. � USBID-License.txt.
 */
#endif

/* ���������, ��� �� ����� �������� VID � PID, ������� ���� ������� ���� (lowbyte),
     ������� ���� (highbyte) */
#ifndef USB_CFG_VENDOR_ID
#   define  USB_CFG_VENDOR_ID   0xc0, 0x16  /* 5824 ����������, ������������ ��� VOTI */
#endif

#ifndef USB_CFG_DEVICE_ID
#   if USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH
#       define USB_CFG_DEVICE_ID    0xdf, 0x05  /* 1503 ����������, ��������������� � ����� ������ PID ��� HID-�� */
#   elif USB_CFG_INTERFACE_CLASS == 2
#       define USB_CFG_DEVICE_ID    0xe1, 0x05  /* 1505 ����������, ��������������� � ����� ������ PID ��� ������� CDC */
#   else
#       define USB_CFG_DEVICE_ID    0xdc, 0x05  /* 1500 ����������, ��������� obdev PID */
#   endif
#endif

/* ������������ Output, Input � DataDirection ����� �� ���� ������ */
#ifndef USB_CFG_IOPORTNAME
#error "You must define USB_CFG_IOPORTNAME in usbconfig.h, see usbconfig-prototype.h"
#endif

#define USBOUT          USB_OUTPORT(USB_CFG_IOPORTNAME)
#define USB_PULLUP_OUT  USB_OUTPORT(USB_CFG_PULLUP_IOPORTNAME)
#define USBIN           USB_INPORT(USB_CFG_IOPORTNAME)
#define USBDDR          USB_DDRPORT(USB_CFG_IOPORTNAME)
#define USB_PULLUP_DDR  USB_DDRPORT(USB_CFG_PULLUP_IOPORTNAME)

#define USBMINUS    USB_CFG_DMINUS_BIT
#define USBPLUS     USB_CFG_DPLUS_BIT
#define USBIDLE     (1<<USB_CFG_DMINUS_BIT) /* ��������, �������������� ��������� J */
#define USBMASK     ((1<<USB_CFG_DPLUS_BIT) | (1<<USB_CFG_DMINUS_BIT))  /* ����� ��� USB ����� I/O */

/* ����������� ��� �������� ������������� �� ������� �������� ��������: */
#define USB_CFG_IOPORT          USB_OUTPORT(USB_CFG_IOPORTNAME)
#ifdef USB_CFG_PULLUP_IOPORTNAME
#define USB_CFG_PULLUP_IOPORT   USB_OUTPORT(USB_CFG_PULLUP_IOPORTNAME)
#endif

#ifndef USB_CFG_EP3_NUMBER  /* ���� �� ������ � usbconfig.h */
#define USB_CFG_EP3_NUMBER  3
#endif

#define USB_BUFSIZE     11  /* PID, 8 ���� ������, 2 ����� CRC */

/* ----- ������� ����� �������� � ����, ���������� �� ������� ���������� 0 ----- */

#ifndef USB_INTR_CFG    /* ������������ ����� ���������� ���� ��������� */
#   if defined  EICRA
#       define USB_INTR_CFG EICRA
#   else
#       define USB_INTR_CFG MCUCR
#   endif
#endif
#ifndef USB_INTR_CFG_SET    /* ������������ ����� ���������� ���� �������� �� ��������� */
#   define USB_INTR_CFG_SET ((1 << ISC00) | (1 << ISC01))    /* ������������� �� ����� (����������) ������� */
#endif
#ifndef USB_INTR_CFG_CLR    /* ������������ ����� ���������� ���� �������� �� ��������� */
#   define USB_INTR_CFG_CLR 0    /* ��� ����� ��� ������� */
#endif

#ifndef USB_INTR_ENABLE     /* ������������ ����� ���������� ���� �������� �� ��������� */
#   if defined GIMSK
#       define USB_INTR_ENABLE  GIMSK
#   elif defined EIMSK
#       define USB_INTR_ENABLE  EIMSK
#   else
#       define USB_INTR_ENABLE  GICR
#   endif
#endif
#ifndef USB_INTR_ENABLE_BIT /* ������������ ����� ���������� ���� �������� �� ��������� */
#   define USB_INTR_ENABLE_BIT  INT0
#endif

#ifndef USB_INTR_PENDING    /* ������������ ����� ���������� ���� �������� �� ��������� */
#   if defined  EIFR
#       define USB_INTR_PENDING EIFR
#   else
#       define USB_INTR_PENDING GIFR
#   endif
#endif
#ifndef USB_INTR_PENDING_BIT    /* ������������ ����� ���������� ���� �������� �� ��������� */
#   define USB_INTR_PENDING_BIT INTF0
#endif

/*
����������� ���� �� �������� ��� ��������� �����:
at90c8534: ��� ISC0?, ��� PORTB, ���������� ����� �������
at86rf401: ��� PORTB, ��� MCUCR � �. �., ������ ������� ������
atmega103: ��� ISC0? (����� ���� ��������� � ���������, ���������� ����� �������)
atmega603: �� ������ � avr-libc
at43usb320, at43usb355, at76c711: ��� ������� USB
at94k: ��� ������ ���...

at90s1200, attiny11, attiny12, attiny15, attiny28: ��� ���������������� �� ����� RAM
*/

/* ------------------------------------------------------------------------- */
/* ------------------- ��������� � ���� ������������ USB ------------------- */
/* ------------------------------------------------------------------------- */

/* �������� ������� USB */
#define USBPID_SETUP    0x2d
#define USBPID_OUT      0xe1
#define USBPID_IN       0x69
#define USBPID_DATA0    0xc3
#define USBPID_DATA1    0x4b

#define USBPID_ACK      0xd2
#define USBPID_NAK      0x5a
#define USBPID_STALL    0x1e

#ifndef USB_INITIAL_DATATOKEN
#define USB_INITIAL_DATATOKEN   USBPID_DATA1
#endif

#ifndef __ASSEMBLER__

typedef struct usbTxStatus{
    volatile uchar   len;
    uchar   buffer[USB_BUFSIZE];
}usbTxStatus_t;

extern usbTxStatus_t   usbTxStatus1, usbTxStatus3;
#define usbTxLen1   usbTxStatus1.len
#define usbTxBuf1   usbTxStatus1.buffer
#define usbTxLen3   usbTxStatus3.len
#define usbTxBuf3   usbTxStatus3.buffer


typedef union usbWord{
    unsigned    word;
    uchar       bytes[2];
}usbWord_t;

typedef struct usbRequest{
    uchar       bmRequestType;
    uchar       bRequest;
    usbWord_t   wValue;
    usbWord_t   wIndex;
    usbWord_t   wLength;
}usbRequest_t;
/* ��� ��������� ������������� 8-�������� ������� setup */
#endif

/* ���� bmRequestType � USB setup:
 * d t t r r r r r, ���
 * d ..... �����������: 0=����->����������, 1=����������->����
 * t ..... ���: 0=�����������, 1=�����, 2=������, 3=���������������
 * r ..... ����������: 0=����������, 1=���������, 2=�������� �����, 3=������
 */

/* �������� ���������� USB setup */
#define USBRQ_RCPT_MASK         0x1f
#define USBRQ_RCPT_DEVICE       0
#define USBRQ_RCPT_INTERFACE    1
#define USBRQ_RCPT_ENDPOINT     2

/* �������� ���� ������� USB */
#define USBRQ_TYPE_MASK         0x60
#define USBRQ_TYPE_STANDARD     (0<<5)
#define USBRQ_TYPE_CLASS        (1<<5)
#define USBRQ_TYPE_VENDOR       (2<<5)

/* �������� ����������� USB: */
#define USBRQ_DIR_MASK              0x80
#define USBRQ_DIR_HOST_TO_DEVICE    (0<<7)
#define USBRQ_DIR_DEVICE_TO_HOST    (1<<7)

/* ����������� ������� USB */
#define USBRQ_GET_STATUS        0
#define USBRQ_CLEAR_FEATURE     1
#define USBRQ_SET_FEATURE       3
#define USBRQ_SET_ADDRESS       5
#define USBRQ_GET_DESCRIPTOR    6
#define USBRQ_SET_DESCRIPTOR    7
#define USBRQ_GET_CONFIGURATION 8
#define USBRQ_SET_CONFIGURATION 9
#define USBRQ_GET_INTERFACE     10
#define USBRQ_SET_INTERFACE     11
#define USBRQ_SYNCH_FRAME       12

/* ��������� ����������� USB */
#define USBDESCR_DEVICE         1
#define USBDESCR_CONFIG         2
#define USBDESCR_STRING         3
#define USBDESCR_INTERFACE      4
#define USBDESCR_ENDPOINT       5
#define USBDESCR_HID            0x21
#define USBDESCR_HID_REPORT     0x22
#define USBDESCR_HID_PHYS       0x23

#define USBATTR_BUSPOWER        0x80
#define USBATTR_SELFPOWER       0x40
#define USBATTR_REMOTEWAKE      0x20

/* ������� USB HID */
#define USBRQ_HID_GET_REPORT    0x01
#define USBRQ_HID_GET_IDLE      0x02
#define USBRQ_HID_GET_PROTOCOL  0x03
#define USBRQ_HID_SET_REPORT    0x09
#define USBRQ_HID_SET_IDLE      0x0a
#define USBRQ_HID_SET_PROTOCOL  0x0b

/* ------------------------------------------------------------------------- */

#endif /* __usbdrv_h_included__ */
