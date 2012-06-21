/* Имя: usbdrvasm.S
 * Проект: AVR USB driver
 * Автор: Christian Starkjohann
 * Перевод: microsin.ru
 * Дата создания: 2007-06-13
 * Табуляция: 4
 * Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 * Лицензия: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
 * Ревизия: $Id$
 */

/*
Основное описание: 
Этот модуль является частью драйвера USB, написанной на ассемблере. Этот файл
содержит главный код (препроцессор и вычисление CRC) и включает файл, зависящий 
от тактовой частоты микроконтроллера.
*/

#include "iarcompat.h"
#ifndef __IAR_SYSTEMS_ASM__
    /* configs for io.h */
#   define __SFR_OFFSET 0
#   define _VECTOR(N)   __vector_ ## N   /* io.h не задает этого для asm */
#   include <avr/io.h> /* определения для регистров I/O CPU и векторы */
#   define macro    .macro  /* определение макроса ассемблера GNU */
#   define endm     .endm   /* конец определения макроса ассемблера GNU */
#endif  /* __IAR_SYSTEMS_ASM__ */
#include "usbdrv.h" /* для общих определений */

/* имена регистров */
#define x1      r16
#define x2      r17
#define shift   r18
#define cnt     r19
#define x3      r20
#define x4      r21
#define bitcnt  r22
#define phase   x4
#define leap    x4

/* Некоторые определения и декларации, зависяие от ассемблера: */

#ifdef __IAR_SYSTEMS_ASM__

#   define nop2     rjmp    $+2 /* прыжок на следующую инструкцию */
#   define XL       r26
#   define XH       r27
#   define YL       r28
#   define YH       r29
#   define ZL       r30
#   define ZH       r31
#   define lo8(x)   LOW(x)
#   define hi8(x)   (((x)>>8) & 0xff)   /* не HIGH чтобы позволить XLINK сделать правильную проверку диапазона */

    extern  usbRxBuf, usbDeviceAddr, usbNewDeviceAddr, usbInputBufOffset
    extern  usbCurrentTok, usbRxLen, usbRxToken, usbTxLen
    extern  usbTxBuf, usbTxStatus1, usbTxStatus3
#   if USB_COUNT_SOF
        extern usbSofCount
#   endif
    public  usbCrc16
    public  usbCrc16Append

    COMMON  INTVEC
#   ifndef USB_INTR_VECTOR
        ORG     INT0_vect
#   else /* USB_INTR_VECTOR */
        ORG     USB_INTR_VECTOR
#       undef   USB_INTR_VECTOR
#   endif /* USB_INTR_VECTOR */
#   define  USB_INTR_VECTOR usbInterruptHandler
    rjmp    USB_INTR_VECTOR
    RSEG    CODE

#else /* __IAR_SYSTEMS_ASM__ */

#   define nop2     rjmp    .+0 /* прыжок на следующую инструкцию */

#   ifndef USB_INTR_VECTOR /* по умолчанию аппаратное прерывание INT0 */
#       define USB_INTR_VECTOR  SIG_INTERRUPT0
#   endif
    .text
    .global USB_INTR_VECTOR
    .type   USB_INTR_VECTOR, @function
    .global usbCrc16
    .global usbCrc16Append
#endif /* __IAR_SYSTEMS_ASM__ */


#if USB_INTR_PENDING < 0x40 /* Это адрес I/O, используйте in и out */
#   define  USB_LOAD_PENDING(reg)   in reg, USB_INTR_PENDING
#   define  USB_STORE_PENDING(reg)  out USB_INTR_PENDING, reg
#else   /* It's a memory address, use lds and sts */
#   define  USB_LOAD_PENDING(reg)   lds reg, USB_INTR_PENDING
#   define  USB_STORE_PENDING(reg)  sts USB_INTR_PENDING, reg
#endif

#define usbTxLen1   usbTxStatus1
#define usbTxBuf1   (usbTxStatus1 + 1)
#define usbTxLen3   usbTxStatus3
#define usbTxBuf3   (usbTxStatus3 + 1)


;----------------------------------------------------------------------------
; Функции-утилиты
;----------------------------------------------------------------------------

#ifdef __IAR_SYSTEMS_ASM__
/* Назначение регистров для usbCrc16 при использовании IAR cc */
/* Система вызовов IAR:
 * Первый параметр передается в r16/r17, второй в r18/r19 и так далее.
 * Вызываемая функция должна сохранить значение регистров r4-r15, r24-r29 (r28/r29 является указателем фрейма)
 * Результат возвращается в r16/r17
 * В случает модели памяти "tiny", указатели являются только 8 бит без дополнения. Таким образом мы передаем аргумент 1 как "16 бит unsigned".
 */
RTMODEL "__rt_version", "3"
/* Строка выше будет генерировать ошибку если поменяется система вызовов cc.
 * Величина 3 выше допустима для IAR 4.10B/W32
 */
#   define argLen   r18 /* аргумент 2 */
#   define argPtrL  r16 /* аргумент 1 */
#   define argPtrH  r17 /* аргумент 1 */

#   define resCrcL  r16 /* результат */
#   define resCrcH  r17 /* результат */

#   define ptrL     ZL
#   define ptrH     ZH
#   define ptr      Z
#   define byte     r22
#   define bitCnt   r19
#   define polyL    r20
#   define polyH    r21
#   define scratch  r23

#else  /* __IAR_SYSTEMS_ASM__ */ 
/* Назначение регистров для usbCrc16 при использовании gcc */
/* Система вызовов gcc:
 * Первый параметр передается в r24/r25, второй в r22/23 и так далее.
 * Вызываемая функция должна сохранить значение регистров r1-r17, r28/r29
 * Результат возвращается в r24/r25
 */
#   define argLen   r22 /* аргумент 2 */
#   define argPtrL  r24 /* аргумент 1 */
#   define argPtrH  r25 /* аргумент 1 */

#   define resCrcL  r24 /* результат */
#   define resCrcH  r25 /* результат */

#   define ptrL     XL
#   define ptrH     XH
#   define ptr      x
#   define byte     r18
#   define bitCnt   r19
#   define polyL    r20
#   define polyH    r21
#   define scratch  r23

#endif

; extern unsigned usbCrc16(unsigned char *data, unsigned char len);
; data: r24/25
; len: r22
; врЕменные переменные:
;   r18: data byte
;   r19: bit counter
;   r20/21: polynomial
;   r23: scratch
;   r24/25: crc-sum
;   r26/27=X: ptr
usbCrc16:
    mov     ptrL, argPtrL
    mov     ptrH, argPtrH
    ldi     resCrcL, 0
    ldi     resCrcH, 0
    ldi     polyL, lo8(0xa001)
    ldi     polyH, hi8(0xa001)
    com     argLen      ; argLen = -argLen - 1
crcByteLoop:
    subi    argLen, -1
    brcc    crcReady    ; модифицированный цикл, чтобы удостовериться, что перенос установится далее
    ld      byte, ptr+
    ldi     bitCnt, -8  ; странный счетчик цикла, чтобы удостовериться, что перенос установится там, где нам нужно
    eor     resCrcL, byte
crcBitLoop:
    ror     resCrcH     ; перенос всегда устанавливается здесь
    ror     resCrcL
    brcs    crcNoXor
    eor     resCrcL, polyL
    eor     resCrcH, polyH
crcNoXor:
    subi    bitCnt, -1
    brcs    crcBitLoop
    rjmp    crcByteLoop
crcReady:
    ret
; Благодарим Reimar Doeffinger за оптимизацию подрограммы CRC!

; extern unsigned usbCrc16Append(unsigned char *data, unsigned char len);
usbCrc16Append:
    rcall   usbCrc16
    st      ptr+, resCrcL
    st      ptr+, resCrcH
    ret

#undef argLen
#undef argPtrL
#undef argPtrH
#undef resCrcL
#undef resCrcH
#undef ptrL
#undef ptrH
#undef ptr
#undef byte
#undef bitCnt
#undef polyL
#undef polyH
#undef scratch


#if USB_CFG_HAVE_MEASURE_FRAME_LENGTH
#ifdef __IAR_SYSTEMS_ASM__
/* Назначение регистров для usbMeasureFrameLength при использовании IAR cc */
/* Система вызовов IAR:
 * Первый параметр передается в r16/r17, второй в r18/r19 и так далее.
 * Вызываемая функция должна сохранить значение регистров r4-r15, r24-r29 (r28/r29
 *  является указателем фрейма)
 * Результат возвращается в r16/r17
 * В случает модели памяти "tiny", указатели являются только 8 бит без дополнения. 
 *  Таким образом мы передаем аргумент 1 как "16 бит unsigned".
 */
#   define resL     r16
#   define resH     r17
#   define cnt16L   r30
#   define cnt16H   r31
#   define cntH     r18

#else  /* __IAR_SYSTEMS_ASM__ */ 
/* Назначение регистров для usbMeasureFrameLength при использовании gcc */
/* Система вызовов gcc:
 * Первый параметр передается в r24/r25, второй в r22/23 и так далее.
 * Вызываемая функция должна сохранить значение регистров r1-r17, r28/r29
 * Результат возвращается в r24/r25
 */
#   define resL     r24
#   define resH     r25
#   define cnt16L   r24
#   define cnt16H   r25
#   define cntH     r26
#endif
#   define cnt16    cnt16L

; extern unsigned usbMeasurePacketLength(void);
; возвращает время между двумя стробами idle в перемножении на 7 тактов CPU
.global usbMeasureFrameLength
usbMeasureFrameLength:
    ldi     cntH, 6         ; ждем ~ 10 мс, чтобы D- == 0
    clr     cnt16L
    clr     cnt16H
usbMFTime16:
    dec     cntH
    breq    usbMFTimeout
usbMFWaitStrobe:            ; первое ожидание D- == 0 (строб idle)
    sbiw    cnt16, 1        ;[0] [6]
    breq    usbMFTime16     ;[2]
    sbic    USBIN, USBMINUS ;[3]
    rjmp    usbMFWaitStrobe ;[4]
usbMFWaitIdle:              ; потом ждем, пока idle не появится снова
    sbis    USBIN, USBMINUS ;1 ждем D- == 1
    rjmp    usbMFWaitIdle   ;2
    ldi     cnt16L, 1       ;1 представляет циклы пока еще
    clr     cnt16H          ;1
usbMFWaitLoop:
    in      cntH, USBIN     ;[0] [7]
    adiw    cnt16, 1        ;[1]
    breq    usbMFTimeout    ;[3]
    andi    cntH, USBMASK   ;[4]
    brne    usbMFWaitLoop   ;[5]
usbMFTimeout:
#if resL != cnt16L
    mov     resL, cnt16L
    mov     resH, cnt16H
#endif
    ret

#undef resL
#undef resH
#undef cnt16
#undef cnt16L
#undef cnt16H
#undef cntH

#endif  /* USB_CFG_HAVE_MEASURE_FRAME_LENGTH */

;----------------------------------------------------------------------------
; Теперь включаем код, который зависит от тактовой частоты микроконтроллера
;----------------------------------------------------------------------------

#ifndef USB_CFG_CLOCK_KHZ
#   define USB_CFG_CLOCK_KHZ 12000
#endif

#if USB_CFG_CLOCK_KHZ == 12000
#   include "usbdrvasm12.inc"
#elif USB_CFG_CLOCK_KHZ == 15000
#   include "usbdrvasm15.inc"
#elif USB_CFG_CLOCK_KHZ == 16000
#   include "usbdrvasm16.inc"
#elif USB_CFG_CLOCK_KHZ == 16500
#   include "usbdrvasm165.inc"
#elif USB_CFG_CLOCK_KHZ == 20000
#   include "usbdrvasm20.inc"
#else
#   error "USB_CFG_CLOCK_KHZ is not one of the supported rates!"
#endif
