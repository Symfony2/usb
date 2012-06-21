/* Имя: usbdrvasm12.inc
 * Проект: AVR USB driver
 * Автор: Christian Starkjohann
 * Перевод: microsin.ru 
 * Дата создания: 2004-12-29
 * Табуляция: 4
 * Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 * Лицензия: GNU GPL v2 (см. License.txt) или проприетарная (CommercialLicense.txt)
 * Ревизия: $Id: usbdrvasm12.inc 483 2008-02-05 15:05:32Z cs $
 */

/* Не линкуйте этот файл! Вместо этого линкуйте usbdrvasm.S, который делает include
 *  файла с нужной реализацией (зависящей от тактовой частоты)!
 */

/*
Основное описание:
Этот файл является 12 МГц версией ассемблерной части драйвера USB. Он требует
кристалла 12 МГц (не керамического резонатора и не калиброванного RC-генератора).

См. usbdrv.h для общего описания драйвера.

Поскольку почти весь код критичен по времени выполнения, не меняйте его, если Вы
не представляете полностью, что делаете! Некоторые части не только требуют максимальное
количесво циклов CPU, но и точное количество циклов!


ВременнЫе ограничения согласно спецификации (в количестве циклов):
объект времени                                            min max    CPUcycles
---------------------------------------------------------------------------
EOP от OUT/SETUP до паттерна синхронизации DATA0 (оба rx) 2   16     16-128
EOP от IN до паттерна синхронизации DATA0 (rx, затем tx)  2   7.5    16-60
от DATAx (rx) до ACK/NAK/STALL (tx)                       2   7.5    16-60
*/

;Узел программного приемника. Строгий выбор времени! Не меняйте, если не можете сохранить тайминг!
;время ответа прерывания: 4 цикла + insn запуск = 7 max если прерывания разрешены
;max допустимая задержка прерывания: 34 цикла -> max 25 циклов запрета прерывания
;max использования стека: [ret(2), YL, SREG, YH, shift, x1, x2, x3, cnt, x4] = 11 байт
;Номера в скобках - максимальное количество циклов начиная с SOF.
USB_INTR_VECTOR:
;порядок сохранения регистров: YL, SREG [sofError], YH, shift, x1, x2, x3, cnt
    push    YL              ;2 [35] push только если нужно синхр. со срезом ASAP
    in      YL, SREG        ;1 [37]
    push    YL              ;2 [39]
;----------------------------------------------------------------------------
; Синхронизация с паттерном sync:
;----------------------------------------------------------------------------
;паттерн sync-байта (D-) от младшего (LSb) до старшего (MSb) бита: 01010100 [1 = ожидание (idle) = J, 0 = K]
;sync от J до среза K во время паттерна sync -- использование самых быстрых циклов
;первая часть не имеет таймаута, поскольку она ожидает IDLE или SE1 (== отключено)
waitForJ:
    sbis    USBIN, USBMINUS ;1 [40] ожидаем D- == 1
    rjmp    waitForJ        ;2
waitForK:
;следующий код приводит к окну выборки 1/4 бита, которое соответствует спецификации.
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
foundK:
;{3, 5} после спада на D- средняя задержка: 4 цикла [нам нужно 4 для центрирования выборки]
;у нас есть 1 бит времени, предназначенный для setup, затем снова производится 
; выборка. Числа в скобках - циклы от центра первого sync (двойное K) бита 
; после инструкции
    push    YH                  ;2 [2]
    lds     YL, usbInputBufOffset;2 [4]
    clr     YH                  ;1 [5]
    subi    YL, lo8(-(usbRxBuf));1 [6]
    sbci    YH, hi8(-(usbRxBuf));1 [7]

    sbis    USBIN, USBMINUS ;1 [8] нам нужно 2 бита K [выборка 1 цикла слишком ранняя]
    rjmp    haveTwoBitsK    ;2 [10]
    pop     YH              ;2 [11] отмена предыдущего push
    rjmp    waitForK        ;2 [13] это не был конец sync, пробуем еще раз
haveTwoBitsK:
;----------------------------------------------------------------------------
; сохраняем регистры и инициализируем переменные, когда мы выбираем первые биты:
;----------------------------------------------------------------------------
    push    shift           ;2 [16]
    push    x1              ;2 [12]
    push    x2              ;2 [14]

    in      x1, USBIN       ;1 [17] <-- выбираем бит 0
    ldi     shift, 0xff     ;1 [18]
    bst     x1, USBMINUS    ;1 [19]
    bld     shift, 0        ;1 [20]
    push    x3              ;2 [22]
    push    cnt             ;2 [24]
    
    in      x2, USBIN       ;1 [25] <-- выбираем бит 1
    ser     x3              ;1 [26] [вставленная инструкция инициализации]
    eor     x1, x2          ;1 [27]
    bst     x1, USBMINUS    ;1 [28]
    bld     shift, 1        ;1 [29]
    ldi     cnt, USB_BUFSIZE;1 [30] [вставленная инструкция инициализации]
    rjmp    rxbit2          ;2 [32]

;----------------------------------------------------------------------------
; Цикл приемника (цифры в скобках - циклы внутри байта после инструкции)
;----------------------------------------------------------------------------

unstuff0:               ;1 (взятый переход)
    andi    x3, ~0x01   ;1 [15]
    mov     x1, x2      ;1 [16] x2 содержит последний выбранный (stuffed) бит
    in      x2, USBIN   ;1 [17] <-- выбираем бит 1 снова
    ori     shift, 0x01 ;1 [18]
    rjmp    didUnstuff0 ;2 [20]

unstuff1:               ;1 (взятый переход)
    mov     x2, x1      ;1 [21] x1 содержит последний выбранный (stuffed) бит
    andi    x3, ~0x02   ;1 [22]
    ori     shift, 0x02 ;1 [23]
    nop                 ;1 [24]
    in      x1, USBIN   ;1 [25] <-- выбираем бит 2 снова
    rjmp    didUnstuff1 ;2 [27]

unstuff2:               ;1 (взятый переход)
    andi    x3, ~0x04   ;1 [29]
    ori     shift, 0x04 ;1 [30]
    mov     x1, x2      ;1 [31] x2 содержит последний выбранный (stuffed) бит
    nop                 ;1 [32]
    in      x2, USBIN   ;1 [33] <-- выборка бита 3
    rjmp    didUnstuff2 ;2 [35]

unstuff3:               ;1 (взятый переход)
    in      x2, USBIN   ;1 [34] <-- выборка stuffed бита 3 [один цикл слишком поздно]
    andi    x3, ~0x08   ;1 [35]
    ori     shift, 0x08 ;1 [36]
    rjmp    didUnstuff3 ;2 [38]

unstuff4:               ;1 (взятый переход)
    andi    x3, ~0x10   ;1 [40]
    in      x1, USBIN   ;1 [41] <-- выборка вставленного бита 4
    ori     shift, 0x10 ;1 [42]
    rjmp    didUnstuff4 ;2 [44]

unstuff5:               ;1 (взятый переход)
    andi    x3, ~0x20   ;1 [48]
    in      x2, USBIN   ;1 [49] <-- выборка вставленного бита 5
    ori     shift, 0x20 ;1 [50]
    rjmp    didUnstuff5 ;2 [52]

unstuff6:               ;1 (взятый переход)
    andi    x3, ~0x40   ;1 [56]
    in      x1, USBIN   ;1 [57] <-- выборка вставленного бита 6
    ori     shift, 0x40 ;1 [58]
    rjmp    didUnstuff6 ;2 [60]

; дополнительные действия, выполняемые на битовых интервалах:
; bit 0:    сохранение, очитка [SE0 здесь ненадежен из-за дрибблинга бит в хабах]
; bit 1:    проверка se0 
; bit 2:    проверка переполнения
; bit 3:    восстановление из задержки [задачи бита 0 отнимают слишком много времени]
; bit 4:    ничего
; bit 5:    ничего
; bit 6:    ничего
; bit 7:    переход, eor
rxLoop:
    eor     x3, shift   ;1 [0] реконструкция: x3 0 в месторасположении бита, которое мы поменяли, 1 для других
    in      x1, USBIN   ;1 [1] <-- выборка бита 0
    st      y+, x3      ;2 [3] сохранение данных
    ser     x3          ;1 [4]
    nop                 ;1 [5]
    eor     x2, x1      ;1 [6]
    bst     x2, USBMINUS;1 [7]
    bld     shift, 0    ;1 [8]
    in      x2, USBIN   ;1 [9] <-- выборка бита 1 (или возможно stuffed-бит 0)
    andi    x2, USBMASK ;1 [10]
    breq    se0         ;1 [11] SE0 проверка для бита 1
    andi    shift, 0xf9 ;1 [12]
didUnstuff0:
    breq    unstuff0    ;1 [13]
    eor     x1, x2      ;1 [14]
    bst     x1, USBMINUS;1 [15]
    bld     shift, 1    ;1 [16]
rxbit2:
    in      x1, USBIN   ;1 [17] <-- выборка бита 2 (или возможно stuffed-бит 1)
    andi    shift, 0xf3 ;1 [18]
    breq    unstuff1    ;1 [19] do remaining work for bit 1
didUnstuff1:
    subi    cnt, 1      ;1 [20]
    brcs    overflow    ;1 [21] loop control
    eor     x2, x1      ;1 [22]
    bst     x2, USBMINUS;1 [23]
    bld     shift, 2    ;1 [24]
    in      x2, USBIN   ;1 [25] <-- выборка бита 3 (или возможно stuffed-бит 2)
    andi    shift, 0xe7 ;1 [26]
    breq    unstuff2    ;1 [27]
didUnstuff2:
    eor     x1, x2      ;1 [28]
    bst     x1, USBMINUS;1 [29]
    bld     shift, 3    ;1 [30]
didUnstuff3:
    andi    shift, 0xcf ;1 [31]
    breq    unstuff3    ;1 [32]
    in      x1, USBIN   ;1 [33] <-- выборка бита 4
    eor     x2, x1      ;1 [34]
    bst     x2, USBMINUS;1 [35]
    bld     shift, 4    ;1 [36]
didUnstuff4:
    andi    shift, 0x9f ;1 [37]
    breq    unstuff4    ;1 [38]
    nop2                ;2 [40]
    in      x2, USBIN   ;1 [41] <-- выборка бита 5
    eor     x1, x2      ;1 [42]
    bst     x1, USBMINUS;1 [43]
    bld     shift, 5    ;1 [44]
didUnstuff5:
    andi    shift, 0x3f ;1 [45]
    breq    unstuff5    ;1 [46]
    nop2                ;2 [48]
    in      x1, USBIN   ;1 [49] <-- выборка бита 6
    eor     x2, x1      ;1 [50]
    bst     x2, USBMINUS;1 [51]
    bld     shift, 6    ;1 [52]
didUnstuff6:
    cpi     shift, 0x02 ;1 [53]
    brlo    unstuff6    ;1 [54]
    nop2                ;2 [56]
    in      x2, USBIN   ;1 [57] <-- выборка бита 7
    eor     x1, x2      ;1 [58]
    bst     x1, USBMINUS;1 [59]
    bld     shift, 7    ;1 [60]
didUnstuff7:
    cpi     shift, 0x04 ;1 [61]
    brsh    rxLoop      ;2 [63] управление циклом
unstuff7:
    andi    x3, ~0x80   ;1 [63]
    ori     shift, 0x80 ;1 [64]
    in      x2, USBIN   ;1 [65] <-- выборка stuffed-бита 7
    nop                 ;1 [66]
    rjmp    didUnstuff7 ;2 [68]

macro POP_STANDARD ; 12 циклов
    pop     cnt
    pop     x3
    pop     x2
    pop     x1
    pop     shift
    pop     YH
endm
macro POP_RETI     ; 5 циклов
    pop     YL
    out     SREG, YL
    pop     YL
endm

#include "asmcommon.inc"

;----------------------------------------------------------------------------
; Передача данных
;----------------------------------------------------------------------------

bitstuff0:                  ;1 (для ветвления)
    eor     x1, x4          ;1
    ldi     x2, 0           ;1
    out     USBOUT, x1      ;1 <-- вывод
    rjmp    didStuff0       ;2 ветвь назад на 2 цикла ранее
bitstuff1:                  ;1 (для ветвления)
    eor     x1, x4          ;1
    rjmp    didStuff1       ;2 мы знаем, что C код чист, переход назад для того, чтобы сделать OUT и вдвинуть 0 (ror) в x2
bitstuff2:                  ;1 (для ветвления)
    eor     x1, x4          ;1
    rjmp    didStuff2       ;2 переход назад на 4 цикла ранее, делаем вывод и вдвигаем 0 в x2
bitstuff3:                  ;1 (для ветвления)
    eor     x1, x4          ;1
    rjmp    didStuff3       ;2 переход назад раньше и ror 0 в x2
bitstuff4:                  ;1 (для ветвления)
    eor     x1, x4          ;1
    ldi     x2, 0           ;1
    out     USBOUT, x1      ;1 <-- вывод
    rjmp    didStuff4       ;2 переназад на 2 цикла ранее

sendNakAndReti:                 ;0 [-19] 19 циклов до появления SOP
    ldi     x3, USBPID_NAK      ;1 [-18]
    rjmp    usbSendX3           ;2 [-16]
sendAckAndReti:                 ;0 [-19] 19 циклов до появления SOP
    ldi     x3, USBPID_ACK      ;1 [-18]
    rjmp    usbSendX3           ;2 [-16]
sendCntAndReti:                 ;0 [-17] 17 циклов до появления SOP
    mov     x3, cnt             ;1 [-16]
usbSendX3:                      ;0 [-16]
    ldi     YL, 20              ;1 [-15] 'x3' находится в R20
    ldi     YH, 0               ;1 [-14]
    ldi     cnt, 2              ;1 [-13]
;   rjmp    usbSendAndReti      проваливаемся

; спецификация USB говорит:
; idle = J
; J = (D+ = 0), (D- = 1) или USBOUT = 0x01
; K = (D+ = 1), (D- = 0) или USBOUT = 0x02
; Спецификация позволяет 7.5 бит от EOP до SOP для ответов (= 60 циклов)

;usbSend:
;указатель на данные в 'Y'
;количество байт в 'cnt' -- включая байт sync
;используются: x1...x4, shift, cnt, Y
;числа в скобках определяет время с тех пор, как первый бит синхронизирующего шаблона (sync pattern) послан
usbSendAndReti:             ;0 [-13] тайминг: 13 циклов пока не появится SOP
    in      x2, USBDDR      ;1 [-12]
    ori     x2, USBMASK     ;1 [-11]
    sbi     USBOUT, USBMINUS;2 [-9] подготовка состояния ожидания (idle); D+ и D- должны быть в 0 (без нагрузочных резисторов (pullups))
    in      x1, USBOUT      ;1 [-8] зеркалирование порта для цикла передачи (tx)
    out     USBDDR, x2      ;1 [-7] <- получение шины
; не нужно инициализировать x2 (история bitstuff (вставки неинформационных бит)), поскольку sync стартует с 0
    push    x4              ;2 [-5]
    ldi     x4, USBMASK     ;1 [-4] маска exor (исключающее ИЛИ)
    ldi     shift, 0x80     ;1 [-3] байт sync - первый из отправляемых байт
txLoop:                     ;       [62]
    sbrs    shift, 0        ;1 [-2] [62]
    eor     x1, x4          ;1 [-1] [63]
    out     USBOUT, x1      ;1 [0] <-- вывод бита 0
    ror     shift           ;1 [1]
    ror     x2              ;1 [2]
didStuff0:
    cpi     x2, 0xfc        ;1 [3]
    brsh    bitstuff0       ;1 [4]
    sbrs    shift, 0        ;1 [5]
    eor     x1, x4          ;1 [6]
    ror     shift           ;1 [7]
didStuff1:
    out     USBOUT, x1      ;1 [8] <-- вывод бита 1
    ror     x2              ;1 [9]
    cpi     x2, 0xfc        ;1 [10]
    brsh    bitstuff1       ;1 [11]
    sbrs    shift, 0        ;1 [12]
    eor     x1, x4          ;1 [13]
    ror     shift           ;1 [14]
didStuff2:
    ror     x2              ;1 [15]
    out     USBOUT, x1      ;1 [16] <-- вывод бита 2
    cpi     x2, 0xfc        ;1 [17]
    brsh    bitstuff2       ;1 [18]
    sbrs    shift, 0        ;1 [19]
    eor     x1, x4          ;1 [20]
    ror     shift           ;1 [21]
didStuff3:
    ror     x2              ;1 [22]
    cpi     x2, 0xfc        ;1 [23]
    out     USBOUT, x1      ;1 [24] <-- вывод бита 3
    brsh    bitstuff3       ;1 [25]
    nop2                    ;2 [27]
    ld      x3, y+          ;2 [29]
    sbrs    shift, 0        ;1 [30]
    eor     x1, x4          ;1 [31]
    out     USBOUT, x1      ;1 [32] <-- вывод бита 4
    ror     shift           ;1 [33]
    ror     x2              ;1 [34]
didStuff4:
    cpi     x2, 0xfc        ;1 [35]
    brsh    bitstuff4       ;1 [36]
    sbrs    shift, 0        ;1 [37]
    eor     x1, x4          ;1 [38]
    ror     shift           ;1 [39]
didStuff5:
    out     USBOUT, x1      ;1 [40] <-- вывод бита 5
    ror     x2              ;1 [41]
    cpi     x2, 0xfc        ;1 [42]
    brsh    bitstuff5       ;1 [43]
    sbrs    shift, 0        ;1 [44]
    eor     x1, x4          ;1 [45]
    ror     shift           ;1 [46]
didStuff6:
    ror     x2              ;1 [47]
    out     USBOUT, x1      ;1 [48] <-- вывод бита 6
    cpi     x2, 0xfc        ;1 [49]
    brsh    bitstuff6       ;1 [50]
    sbrs    shift, 0        ;1 [51]
    eor     x1, x4          ;1 [52]
    ror     shift           ;1 [53]
didStuff7:
    ror     x2              ;1 [54]
    cpi     x2, 0xfc        ;1 [55]
    out     USBOUT, x1      ;1 [56] <-- вывод бита 7
    brsh    bitstuff7       ;1 [57]
    mov     shift, x3       ;1 [58]
    dec     cnt             ;1 [59]
    brne    txLoop          ;1/2 [60/61]
;make SE0:
    cbr     x1, USBMASK     ;1 [61] подготовка SE0 [спецификация говорит, что EOP может быть от 15 до 18 циклов]
    pop     x4              ;2 [63]
;теперь числа в скобках - количество циклов от начала SE0
    out     USBOUT, x1      ;1 [0] <-- вывод SE0 -- теперь от 2 бит = 16 циклов пока шина в состоянии ожидания (idle)
;2006-03-06: перенесена передача нового адреса в usbDeviceAddr из кода C в ассемблер:
;установка адреса только после того, как пакет данных отправлен, не после рукопожатия (handshake)
    lds     x2, usbNewDeviceAddr;2 [2]
    lsl     x2;             ;1 [3] мы сравниваем с влево сдвинутым адресом
    subi    YL, 20 + 2      ;1 [4] только назначает адрес на пакеты данных, нет ACK/NAK в x3
    sbci    YH, 0           ;1 [5]
    breq    skipAddrAssign  ;2 [7]
    sts     usbDeviceAddr, x2; если не пропущено: SE0 на один цикл длиннее
skipAddrAssign:
;конец передачи usbDeviceAddress
    ldi     x2, 1<<USB_INTR_PENDING_BIT;1 [8] int0 произошло во время TX -- очистка флага ожидания очереди
    USB_STORE_PENDING(x2)   ;1 [9]
    ori     x1, USBIDLE     ;1 [10]
    in      x2, USBDDR      ;1 [11]
    cbr     x2, USBMASK     ;1 [12] установка обоих ножек на ввод
    mov     x3, x1          ;1 [13]
    cbr     x3, USBMASK     ;1 [14] конфигурируем отсутствие нагрузочных резисторов (pullup) на обоих ножках
    out     USBOUT, x1      ;1 [15] <-- вывод J (idle) -- окончание SE0 (сигнал EOP)
    out     USBDDR, x2      ;1 [16] <-- теперь освобождение шины
    out     USBOUT, x3      ;1 [17] <-- убедимся в том, что нет активных нагрузочных (pull-up) резисторов
    rjmp    doReturn

bitstuff5:                  ;1 (для ветвления)
    eor     x1, x4          ;1
    rjmp    didStuff5       ;2 тот же прием, как и в bitstuff1...
bitstuff6:                  ;1 (для ветвления)
    eor     x1, x4          ;1
    rjmp    didStuff6       ;2 тот же прием, как и выше...
bitstuff7:                  ;1 (для ветвления)
    eor     x1, x4          ;1
    rjmp    didStuff7       ;2 тот же прием, как и выше...
