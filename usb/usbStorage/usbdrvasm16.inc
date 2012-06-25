/* Имя: usbdrvasm16.inc
 * Проект: драйвер AVR USB
 * Автор: Christian Starkjohann
 * Перевод: microsin.ru 
 * Дата создания: 2007-06-15
 * Табуляция: 4
 * Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 * Лицензия: GNU GPL v2 (см. License.txt) или проприетарная (CommercialLicense.txt)
 * Ревизия: $Id: usbdrvasm16.inc 607 2008-05-13 15:57:28Z cs $
 */

/* Не линкуйте этот файл! Вместо этого линкуйте usbdrvasm.S, который делает include
 *  файла с нужной реализацией (зависящей от тактовой частоты)!
 */

/*
Основное описание:
Этот файл является 16 МГц версией ассемблерной части драйвера USB. Он требует
кристалла 16 МГц (не керамического резонатора и не калиброванного RC-генератора).

См. usbdrv.h для общего описания драйвера.

Поскольку почти весь код критичен по времени выполнения, не меняйте его, если Вы
не представляете полностью, что делаете! Некоторые части не только требуют максимальное
количесво циклов CPU, но и точное количество циклов!
*/

;максимальное использование стека: [ret(2), YL, SREG, YH, bitcnt, shift, x1, x2, x3, x4, cnt] = 12 байт
;номинальная частота: 16 МГц -> 10.6666666 циклов на бит, 85.333333333 циклов на байт
; Числа в скобках означают количество циклов, подсчитанных относительно центра последнего бита sync
; когда инструкция стартует

USB_INTR_VECTOR:
;порядок сохранения регистров: YL, SREG YH, [sofError], bitcnt, shift, x1, x2, x3, x4, cnt
    push    YL                  ;[-25] push только если нужно синхр. со срезом ASAP
    in      YL, SREG            ;[-23]
    push    YL                  ;[-22]
    push    YH                  ;[-20]
;----------------------------------------------------------------------------
; Синхронизация с паттерном sync:
;----------------------------------------------------------------------------
;паттерн sync-байта (D-) от младшего (LSb) до старшего (MSb) бита: 01010100 [1 = ожидание (idle) = J, 0 = K]
;sync от J до среза K во время паттерна sync -- использование самых быстрых циклов
;первая часть не имеет таймаута, поскольку она ожидает IDLE или SE1 (== отключено)
waitForJ:
    sbis    USBIN, USBMINUS     ;[-18] ожидание, когда D- == 1
    rjmp    waitForJ
waitForK:
;Cледующий код приводит к окну выборки < 1/4 бита, которое соответствует спецификации.
    sbis    USBIN, USBMINUS     ;[-15]
    rjmp    foundK              ;[-14]
    sbis    USBIN, USBMINUS
    rjmp    foundK
    sbis    USBIN, USBMINUS
    rjmp    foundK
    sbis    USBIN, USBMINUS
    rjmp    foundK
    sbis    USBIN, USBMINUS
    rjmp    foundK
    sbis    USBIN, USBMINUS
    rjmp    foundK
#if USB_COUNT_SOF
    lds     YL, usbSofCount
    inc     YL
    sts     usbSofCount, YL
#endif  /* USB_COUNT_SOF */
    rjmp    sofError
foundK:                         ;[-12]
;{3, 5} после спада на D- средняя задержка: 4 цикла [нам нужно 5 для центрирования выборки]
;у нас есть 1 бит времени, предназначенный для setup, затем снова производится 
; выборка. Числа в скобках - циклы от центра первого sync (двойное K) бита 
; после инструкции
    push    bitcnt              ;[-12]
;   [---]                       ;[-11]
    lds     YL, usbInputBufOffset;[-10]
;   [---]                       ;[-9]
    clr     YH                  ;[-8]
    subi    YL, lo8(-(usbRxBuf));[-7] [инициализация цикла rx]
    sbci    YH, hi8(-(usbRxBuf));[-6] [инициализация цикла rx]
    push    shift               ;[-5]
;   [---]                       ;[-4]
    ldi     bitcnt, 0x55        ;[-3] [инициализация цикла rx]
    sbis    USBIN, USBMINUS     ;[-2] нам нужно 2 бита K (выборка 2 цикла слишком ранняя)
    rjmp    haveTwoBitsK        ;[-1]
    pop     shift               ;[0] отмена предыдущего push
    pop     bitcnt              ;[2] отмена предыдущего push
    rjmp    waitForK            ;[4] это не был конец sync, пробуем еще раз
; Цикл целиком от waitForK до rjmp waitForK выше не должен превышать два бита времени
;  (= 21 цикл).

;----------------------------------------------------------------------------
; сохраняем регистры и инициализируем переменные, когда мы выбираем первые биты:
;----------------------------------------------------------------------------
haveTwoBitsK:
    push    x1              ;[1]
    push    x2              ;[3]
    push    x3              ;[5]
    ldi     shift, 0        ;[7]
    ldi     x3, 1<<4        ;[8] [инициализация цикла rx] первая выборка - инверсный бит, компенсируем это
    push    x4              ;[9] == прыжок

    in      x1, USBIN       ;[11] <-- выборка бита 0
    andi    x1, USBMASK     ;[12]
    bst     x1, USBMINUS    ;[13]
    bld     shift, 7        ;[14]
    push    cnt             ;[15]
    ldi     leap, 0         ;[17] [инициализация цикла rx]
    ldi     cnt, USB_BUFSIZE;[18] [инициализация цикла rx]
    rjmp    rxbit1          ;[19] достигаем до [21]

;----------------------------------------------------------------------------
; Цикл приемника (числа в скобках показывают циклы внутри байта после инструкции)
;----------------------------------------------------------------------------

unstuff6:
    andi    x2, USBMASK ;[03]
    ori     x3, 1<<6    ;[04] больше не будем сдвигать
    andi    shift, ~0x80;[05]
    mov     x1, x2      ;[06] выбранный бит 7 на самом деле выбранный заново бит 6
    subi    leap, 3     ;[07] поскольку это короткий (10 циклов) бит, устанавливаем бит прыжка
    rjmp    didUnstuff6 ;[08]

unstuff7:
    ori     x3, 1<<7    ;[09] больше не будем сдвигать
    in      x2, USBIN   ;[00] [10]  выборка заново бита 7
    andi    x2, USBMASK ;[01]
    andi    shift, ~0x80;[02]
    subi    leap, 3     ;[03] поскольку это короткий (10 циклов) бит, устанавливаем бит прыжка
    rjmp    didUnstuff7 ;[04]

unstuffEven:
    ori     x3, 1<<6    ;[09] будет сдвинуто вправо 6 раз для бита 0
    in      x1, USBIN   ;[00] [10]
    andi    shift, ~0x80;[01]
    andi    x1, USBMASK ;[02]
    breq    se0         ;[03]
    subi    leap, 3     ;[04] поскольку это короткий (10 циклов) бит, устанавливаем бит прыжка
    nop                 ;[05]
    rjmp    didUnstuffE ;[06]

unstuffOdd:
    ori     x3, 1<<5    ;[09] будет сдвинуто вправо 4 раза для бита 1
    in      x2, USBIN   ;[00] [10]
    andi    shift, ~0x80;[01]
    andi    x2, USBMASK ;[02]
    breq    se0         ;[03]
    subi    leap, 3     ;[04] поскольку это короткий (10 циклов) бит, устанавливаем бит прыжка
    nop                 ;[05]
    rjmp    didUnstuffO ;[06]

rxByteLoop:
    andi    x1, USBMASK ;[03]
    eor     x2, x1      ;[04]
    subi    leap, 1     ;[05]
    brpl    skipLeap    ;[06]
    subi    leap, -3    ;1 один прыжковый цикл каждый 3-й байт -> 85 + 1/3 циклов на байт
    nop                 ;1
skipLeap:
    subi    x2, 1       ;[08]
    ror     shift       ;[09]
didUnstuff6:
    cpi     shift, 0xfc ;[10]
    in      x2, USBIN   ;[00] [11] <-- выборка бита 7
    brcc    unstuff6    ;[01]
    andi    x2, USBMASK ;[02]
    eor     x1, x2      ;[03]
    subi    x1, 1       ;[04]
    ror     shift       ;[05]
didUnstuff7:
    cpi     shift, 0xfc ;[06]
    brcc    unstuff7    ;[07]
    eor     x3, shift   ;[08] реконструкция: x3 1 в месторасположении бита, которое мы поменяли, 0 для других
    st      y+, x3      ;[09] сохранение данных
rxBitLoop:
    in      x1, USBIN   ;[00] [11] <-- выборка бит 0/2/4
    andi    x1, USBMASK ;[01]
    eor     x2, x1      ;[02]
    andi    x3, 0x3f    ;[03] высший больше всего зарезервировали 2 бита для 6 и 7
    subi    x2, 1       ;[04]
    ror     shift       ;[05]
    cpi     shift, 0xfc ;[06]
    brcc    unstuffEven ;[07]
didUnstuffE:
    lsr     x3          ;[08]
    lsr     x3          ;[09]
rxbit1:
    in      x2, USBIN   ;[00] [10] <-- выборка бит 1/3/5
    andi    x2, USBMASK ;[01]
    breq    se0         ;[02]
    eor     x1, x2      ;[03]
    subi    x1, 1       ;[04]
    ror     shift       ;[05]
    cpi     shift, 0xfc ;[06]
    brcc    unstuffOdd  ;[07]
didUnstuffO:
    subi    bitcnt, 0xab;[08] == addi 0x55, 0x55 = 0x100/3
    brcs    rxBitLoop   ;[09]

    subi    cnt, 1      ;[10]
    in      x1, USBIN   ;[00] [11] <-- выборка бита 6
    brcc    rxByteLoop  ;[01]
    rjmp    overflow

macro POP_STANDARD ; 14 циклов
    pop     cnt
    pop     x4
    pop     x3
    pop     x2
    pop     x1
    pop     shift
    pop     bitcnt
endm
macro POP_RETI     ; 7 циклов
    pop     YH
    pop     YL
    out     SREG, YL
    pop     YL
endm

#include "asmcommon.inc"

; спецификация USB говорит:
; idle = J
; J = (D+ = 0), (D- = 1)
; K = (D+ = 1), (D- = 0)
; Спецификация позволяет 7.5 бит от EOP до SOP для ответов

bitstuffN:
    eor     x1, x4          ;[5]
    ldi     x2, 0           ;[6]
    nop2                    ;[7]
    nop                     ;[9]
    out     USBOUT, x1      ;[10] <-- вывод
    rjmp    didStuffN       ;[0]
    
bitstuff6:
    eor     x1, x4          ;[5]
    ldi     x2, 0           ;[6] флаг переноса ноль, так как далее будет команда brcc
    rol     shift           ;[7] компенсация сдвига ror на ветке назначения
    rjmp    didStuff6       ;[8]

bitstuff7:
    ldi     x2, 0           ;[2] флаг переноса ноль, так как далее будет команда brcc
    rjmp    didStuff7       ;[3]


sendNakAndReti:
    ldi     x3, USBPID_NAK  ;[-18]
    rjmp    sendX3AndReti   ;[-17]
sendAckAndReti:
    ldi     cnt, USBPID_ACK ;[-17]
sendCntAndReti:
    mov     x3, cnt         ;[-16]
sendX3AndReti:
    ldi     YL, 20          ;[-15] x3==r20 адрес равен 20
    ldi     YH, 0           ;[-14]
    ldi     cnt, 2          ;[-13]
;   rjmp    usbSendAndReti      проваливаемся

;usbSend:
;указатель на данные в 'Y'
;количество байт в 'cnt' -- включая байт sync [диапазон 2 ... 12]
;используются: x1...x4, btcnt, shift, cnt, Y
;числа в скобках определяет время с тех пор, как первый бит синхронизирующего шаблона (sync pattern) послан
;Нам не нужно точно соответствовать скорости передачи, потому что спецификация все равно требует точности 
; только 1.5%.
usbSendAndReti:             ; 12 циклов пока не появится SOP
    in      x2, USBDDR      ;[-12]
    ori     x2, USBMASK     ;[-11]
    sbi     USBOUT, USBMINUS;[-10] подготовка состояния ожидания (idle); D+ и D- должны быть в 0 (без нагрузочных резисторов (pullups))
    in      x1, USBOUT      ;[-8] зеркалирование порта для цикла передачи (tx)
    out     USBDDR, x2      ;[-7] <- получение шины
; не нужно инициализировать x2 (история bitstuff (вставки неинформационных бит)), поскольку sync стартует с 0
    ldi     x4, USBMASK     ;[-6] маска exor (исключающее ИЛИ)
    ldi     shift, 0x80     ;[-5] байт sync - первый из отправляемых байт
txByteLoop:
    ldi     bitcnt, 0x35    ;[-4] [6] двоичное 0011 0101
txBitLoop:
    sbrs    shift, 0        ;[-3] [7]
    eor     x1, x4          ;[-2] [8]
    out     USBOUT, x1      ;[-1] [9] <-- вывод N
    ror     shift           ;[0] [10]
    ror     x2              ;[1]
didStuffN:
    cpi     x2, 0xfc        ;[2]
    brcc    bitstuffN       ;[3]
    lsr     bitcnt          ;[4]
    brcc    txBitLoop       ;[5]
    brne    txBitLoop       ;[6]

    sbrs    shift, 0        ;[7]
    eor     x1, x4          ;[8]
didStuff6:
    out     USBOUT, x1      ;[-1] [9] <-- вывод 6
    ror     shift           ;[0] [10]
    ror     x2              ;[1]
    cpi     x2, 0xfc        ;[2]
    brcc    bitstuff6       ;[3]
    ror     shift           ;[4]
didStuff7:
    ror     x2              ;[5]
    sbrs    x2, 7           ;[6]
    eor     x1, x4          ;[7]
    nop                     ;[8]
    cpi     x2, 0xfc        ;[9]
    out     USBOUT, x1      ;[-1][10] <-- вывод 7
    brcc    bitstuff7       ;[0] [11]
    ld      shift, y+       ;[1]
    dec     cnt             ;[3]
    brne    txByteLoop      ;[4]
;make SE0:
    cbr     x1, USBMASK     ;[5] подготовка SE0 [спецификация говорит, что EOP может быть от 21 до 25 циклов]
    lds     x2, usbNewDeviceAddr;[6]
    lsl     x2              ;[8] мы сравниваем с влево сдвинутым адресом
    subi    YL, 20 + 2      ;[9] только назначает адрес на пакеты данных, нет ACK/NAK в x3
    sbci    YH, 0           ;[10]
    out     USBOUT, x1      ;[11] <-- вывод SE0-- теперь от 2 бит == 20 циклов пока шина в состоянии ожидания (idle)
;2006-03-06: перенесена передача нового адреса в usbDeviceAddr из кода C в ассемблер:
;установка адреса только после того, как пакет данных отправлен, не после рукопожатия (handshake)
    breq    skipAddrAssign  ;[0]
    sts     usbDeviceAddr, x2; если не пропущено: SE0 на один цикл длиннее
skipAddrAssign:
;конец передачи usbDeviceAddress
    ldi     x2, 1<<USB_INTR_PENDING_BIT;[2] int0 произошло во время TX -- очистка флага ожидания очереди
    USB_STORE_PENDING(x2)   ;[3]
    ori     x1, USBIDLE     ;[4]
    in      x2, USBDDR      ;[5]
    cbr     x2, USBMASK     ;[6] установка обоих ножек на ввод
    mov     x3, x1          ;[7]
    cbr     x3, USBMASK     ;[8] конфигурируем отсутствие нагрузочных резисторов (pullup) на обоих ножках
    ldi     x4, 4           ;[9]
se0Delay:
    dec     x4              ;[10] [13] [16] [19]
    brne    se0Delay        ;[11] [14] [17] [20]
    out     USBOUT, x1      ;[21] <-- вывод J (idle) -- окончание SE0 (сигнал EOP)
    out     USBDDR, x2      ;[22] <-- теперь освобождение шины
    out     USBOUT, x3      ;[23] <-- убедимся в том, что нет активных нагрузочных (pull-up) резисторов
    rjmp    doReturn
