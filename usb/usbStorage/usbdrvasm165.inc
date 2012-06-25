/* Имя: usbdrvasm165.inc
 * Проект: AVR USB driver
 * Автор: Christian Starkjohann
 * Перевод: microsin.ru 
 * Дата создания: 2007-04-22
 * Табуляция: 4
 * Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 * Лицензия: GNU GPL v2 (см. License.txt) или проприетарная (CommercialLicense.txt)
 * Ревизия: $Id: usbdrvasm165.inc 607 2008-05-13 15:57:28Z cs $
 */

/* Не линкуйте этот файл! Вместо этого линкуйте usbdrvasm.S, который делает include
 *  файла с нужной реализацией (зависящей от тактовой частоты)!
 */

/*
Основное описание:
Этот файл является 16 МГц версией ассемблерной части драйвера USB. Он предназначен
для ATTiny45 и подобных микроконтроллеров, работающих на частоте 16.5 МГц внутреннего
RC-генератора. Эта версия содержит фазовую автоподстройку частоты в программе 
приемника, чтобы справиться с девиациями тактовой частоты +/- 1%.

См. usbdrv.h для общего описания драйвера.

Поскольку почти весь код критичен по времени выполнения, не меняйте его, если Вы
не представляете полностью, что делаете! Некоторые части не только требуют максимальное
количесво циклов CPU, но и точное количество циклов!
*/

;Узел программного приемника данных. Строгий выбор времени! Не меняйте, если не можете сохранить тайминг!
;время ответа прерывания: 4 цикла + insn запуск = 7 max если прерывания разрешены
;max допустимая задержка прерывания: 59 циклов -> max 52 циклов запрета прерывания
;max использования стека: [ret(2), r0, SREG, YL, YH, shift, x1, x2, x3, x4, cnt] = 12 байт
;номинальная частота: 16.5 МГц -> 11 циклов на бит
; 16.3125 МГц < F_CPU < 16.6875 МГц (+/- 1.1%)
; Номера в скобках - такты отсчитанные от центра боследнего бита sync
; когда инструкция стартует


USB_INTR_VECTOR:
;порядок сохранения регистров: YL, SREG [sofError], r0, YH, shift, x1, x2, x3, x4, cnt
    push    YL                  ;[-23] push только если нужно синхр. со срезом ASAP
    in      YL, SREG            ;[-21]
    push    YL                  ;[-20]
;----------------------------------------------------------------------------
; Синхронизация с паттерном sync:
;----------------------------------------------------------------------------
;паттерн sync-байта (D-) от младшего (LSb) до старшего (MSb) бита: 01010100 [1 = ожидание (idle) = J, 0 = K]
;sync от J до среза K во время паттерна sync -- использование самых быстрых циклов
;первая часть не имеет таймаута, поскольку она ожидает IDLE или SE1 (== отключено)
waitForJ:
    sbis    USBIN, USBMINUS     ;[-18] ожидаем D- == 1
    rjmp    waitForJ
waitForK:
;следующий код приводит к окну выборки < 1/4 бита, которое соответствует спецификации.
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
; выборка. Числа в скобках - циклы от центра первого sync (двойное K) бита 
; после инструкции
    push    r0                  ;[-12]
;   [---]                       ;[-11]
    push    YH                  ;[-10]
;   [---]                       ;[-9]
    lds     YL, usbInputBufOffset;[-8]
;   [---]                       ;[-7]
    clr     YH                  ;[-6]
    subi    YL, lo8(-(usbRxBuf));[-5] [инициализация цикла rx]
    sbci    YH, hi8(-(usbRxBuf));[-4] [инициализация цикла rx]
    mov     r0, x2              ;[-3] [инициализация цикла rx]
    sbis    USBIN, USBMINUS     ;[-2] нам нужно 2 бита K (выборка 2 цикла слишком ранняя)
    rjmp    haveTwoBitsK        ;[-1]
    pop     YH                  ;[0] отмена предыдущего push
    pop     r0                  ;[2]
    rjmp    waitForK            ;[4] это не был конец sync, пробуем еще раз
; Цикл целиком от waitForK до rjmp waitForK выше не должен превышать два бита времени
;  (= 22 цикла).

;----------------------------------------------------------------------------
; сохраняем регистры и инициализируем переменные, когда мы выбираем первые биты:
;----------------------------------------------------------------------------
haveTwoBitsK:               ;[1]
    push    shift           ;[1]
    push    x1              ;[3]
    push    x2              ;[5]
    push    x3              ;[7]
    ldi     shift, 0xff     ;[9] [инициализация цикла rx]
    ori     x3, 0xff        ;[10] [инициализация цикла rx] == установка x3, очистка флага нуля

    in      x1, USBIN       ;[11] <-- выборка бита 0
    bst     x1, USBMINUS    ;[12]
    bld     shift, 0        ;[13]
    push    x4              ;[14] == фаза
;   [---]                   ;[15]
    push    cnt             ;[16]
;   [---]                   ;[17]
    ldi     phase, 0        ;[18] [инициализация цикла rx]
    ldi     cnt, USB_BUFSIZE;[19] [инициализация цикла rx]
    rjmp    rxbit1          ;[20]
;   [---]                   ;[21]

;----------------------------------------------------------------------------
; Цикл приемника (числа в скобках показывают циклы внутри байта после инструкции)
;----------------------------------------------------------------------------
/*
байт-ориентированные операции во время цикла:
bit 0: сохранение данных
bit 1: проверка SE0
bit 2: проверка переполнения
bit 3: подхват
bit 4: rjmp для достижения обусловленного диапазона перехода
bit 5: PLL (Phase Locked Loop, ФАПЧ - фазовая автоподстройка частоты)
bit 6: подхват
bit 7: переход, фиксация bitstuff
; 87 [+ 2] цикла
------------------------------------------------------------------
*/
continueWithBit5:
    in      x2, USBIN       ;[055] <-- бит 5
    eor     r0, x2          ;[056]
    or      phase, r0       ;[057]
    sbrc    phase, USBMINUS ;[058]
    lpm                     ;[059] дополнительный nop3; меняет r0
    in      phase, USBIN    ;[060] <-- фаза
    eor     x1, x2          ;[061]
    bst     x1, USBMINUS    ;[062]
    bld     shift, 5        ;[063]
    andi    shift, 0x3f     ;[064]
    in      x1, USBIN       ;[065] <-- бит 6
    breq    unstuff5        ;[066] *** unstuff выход
    eor     phase, x1       ;[067]
    eor     x2, x1          ;[068]
    bst     x2, USBMINUS    ;[069]
    bld     shift, 6        ;[070]
didUnstuff6:                ;[   ]
    in      r0, USBIN       ;[071] <-- фаза
    cpi     shift, 0x02     ;[072]
    brlo    unstuff6        ;[073] *** unstuff выход
didUnstuff5:                ;[   ]
    nop2                    ;[074]
;   [---]                   ;[075]
    in      x2, USBIN       ;[076] <-- бит 7
    eor     x1, x2          ;[077]
    bst     x1, USBMINUS    ;[078]
    bld     shift, 7        ;[079]
didUnstuff7:                ;[   ]
    eor     r0, x2          ;[080]
    or      phase, r0       ;[081]
    in      r0, USBIN       ;[082] <-- фаза
    cpi     shift, 0x04     ;[083]
    brsh    rxLoop          ;[084]
;   [---]                   ;[085]
unstuff7:                   ;[   ]
    andi    x3, ~0x80       ;[085]
    ori     shift, 0x80     ;[086]
    in      x2, USBIN       ;[087] <-- выборка stuffed бита 7
    nop                     ;[088]
    rjmp    didUnstuff7     ;[089]
;   [---]                   ;[090]
                            ;[080]

unstuff5:                   ;[067]
    eor     phase, x1       ;[068]
    andi    x3, ~0x20       ;[069]
    ori     shift, 0x20     ;[070]
    in      r0, USBIN       ;[071] <-- фаза
    mov     x2, x1          ;[072]
    nop                     ;[073]
    nop2                    ;[074]
;   [---]                   ;[075]
    in      x1, USBIN       ;[076] <-- бит 6
    eor     r0, x1          ;[077]
    or      phase, r0       ;[078]
    eor     x2, x1          ;[079]
    bst     x2, USBMINUS    ;[080]
    bld     shift, 6        ;[081] не нужно проверять, только что это было
    in      r0, USBIN       ;[082] <-- phase
    rjmp    didUnstuff5     ;[083]
;   [---]                   ;[084]
                            ;[074]

unstuff6:                   ;[074]
    andi    x3, ~0x40       ;[075]
    in      x1, USBIN       ;[076] <-- снова бит 6
    ori     shift, 0x40     ;[077]
    nop2                    ;[078]
;   [---]                   ;[079]
    rjmp    didUnstuff6     ;[080]
;   [---]                   ;[081]
                            ;[071]

unstuff0:                   ;[013]
    eor     r0, x2          ;[014]
    or      phase, r0       ;[015]
    andi    x2, USBMASK     ;[016] проверка SE0
    in      r0, USBIN       ;[017] <-- фаза
    breq    didUnstuff0     ;[018] прямой переход к se0 был бы слишком длинным
    andi    x3, ~0x01       ;[019]
    ori     shift, 0x01     ;[020]
    mov     x1, x2          ;[021] перемещаем имеющуюся выборку
    in      x2, USBIN       ;[022] <-- снова бит 1
    rjmp    didUnstuff0     ;[023]
;   [---]                   ;[024]
                            ;[014]

unstuff1:                   ;[024]
    eor     r0, x1          ;[025]
    or      phase, r0       ;[026]
    andi    x3, ~0x02       ;[027]
    in      r0, USBIN       ;[028] <-- фаза
    ori     shift, 0x02     ;[029]
    mov     x2, x1          ;[030]
    rjmp    didUnstuff1     ;[031]
;   [---]                   ;[032]
                            ;[022]

unstuff2:                   ;[035]
    eor     r0, x2          ;[036]
    or      phase, r0       ;[037]
    andi    x3, ~0x04       ;[038]
    in      r0, USBIN       ;[039] <-- фаза
    ori     shift, 0x04     ;[040]
    mov     x1, x2          ;[041]
    rjmp    didUnstuff2     ;[042]
;   [---]                   ;[043]
                            ;[033]

unstuff3:                   ;[043]
    in      x2, USBIN       ;[044] <-- снова бит 3
    eor     r0, x2          ;[045]
    or      phase, r0       ;[046]
    andi    x3, ~0x08       ;[047]
    ori     shift, 0x08     ;[048]
    nop                     ;[049]
    in      r0, USBIN       ;[050] <-- фаза
    rjmp    didUnstuff3     ;[051]
;   [---]                   ;[052]
                            ;[042]

unstuff4:                   ;[053]
    andi    x3, ~0x10       ;[054]
    in      x1, USBIN       ;[055] <-- снова бит 4
    ori     shift, 0x10     ;[056]
    rjmp    didUnstuff4     ;[057]
;   [---]                   ;[058]
                            ;[048]

rxLoop:                     ;[085]
    eor     x3, shift       ;[086] реконструкция: x3 0 в месторасположении бита, которое мы поменяли, 1 для других
    in      x1, USBIN       ;[000] <-- бит 0
    st      y+, x3          ;[001]
;   [---]                   ;[002]
    eor     r0, x1          ;[003]
    or      phase, r0       ;[004]
    eor     x2, x1          ;[005]
    in      r0, USBIN       ;[006] <-- фаза
    ser     x3              ;[007]
    bst     x2, USBMINUS    ;[008]
    bld     shift, 0        ;[009]
    andi    shift, 0xf9     ;[010]
rxbit1:                     ;[   ]
    in      x2, USBIN       ;[011] <-- бит 1
    breq    unstuff0        ;[012] *** unstuff выход
    andi    x2, USBMASK     ;[013] SE0 проверка для бита 1
didUnstuff0:                ;[   ] Z устанавливается только если мы детектировали SE0 в bitstuff
    breq    se0             ;[014]
    eor     r0, x2          ;[015]
    or      phase, r0       ;[016]
    in      r0, USBIN       ;[017] <-- фаза
    eor     x1, x2          ;[018]
    bst     x1, USBMINUS    ;[019]
    bld     shift, 1        ;[020]
    andi    shift, 0xf3     ;[021]
didUnstuff1:                ;[   ]
    in      x1, USBIN       ;[022] <-- бит 2
    breq    unstuff1        ;[023] *** unstuff выход
    eor     r0, x1          ;[024]
    or      phase, r0       ;[025]
    subi    cnt, 1          ;[026] проверка переполнения
    brcs    overflow        ;[027]
    in      r0, USBIN       ;[028] <-- фаза
    eor     x2, x1          ;[029]
    bst     x2, USBMINUS    ;[030]
    bld     shift, 2        ;[031]
    andi    shift, 0xe7     ;[032]
didUnstuff2:                ;[   ]
    in      x2, USBIN       ;[033] <-- бит 3
    breq    unstuff2        ;[034] *** unstuff выход
    eor     r0, x2          ;[035]
    or      phase, r0       ;[036]
    eor     x1, x2          ;[037]
    bst     x1, USBMINUS    ;[038]
    in      r0, USBIN       ;[039] <-- фаза
    bld     shift, 3        ;[040]
    andi    shift, 0xcf     ;[041]
didUnstuff3:                ;[   ]
    breq    unstuff3        ;[042] *** unstuff выход
    nop                     ;[043]
    in      x1, USBIN       ;[044] <-- бит 4
    eor     x2, x1          ;[045]
    bst     x2, USBMINUS    ;[046]
    bld     shift, 4        ;[047]
didUnstuff4:                ;[   ]
    eor     r0, x1          ;[048]
    or      phase, r0       ;[049]
    in      r0, USBIN       ;[050] <-- фаза
    andi    shift, 0x9f     ;[051]
    breq    unstuff4        ;[052] *** unstuff выход
    rjmp    continueWithBit5;[053]
;   [---]                   ;[054]

macro POP_STANDARD ; 16 циклов
    pop     cnt
    pop     x4
    pop     x3
    pop     x2
    pop     x1
    pop     shift
    pop     YH
    pop     r0
endm
macro POP_RETI     ; 5 циклов
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

bitstuff7:
    eor     x1, x4          ;[4]
    ldi     x2, 0           ;[5]
    nop2                    ;[6] C равен нулю (инструкция brcc)
    rjmp    didStuff7       ;[8]

bitstuffN:
    eor     x1, x4          ;[5]
    ldi     x2, 0           ;[6]
    lpm                     ;[7] 3 цикла NOP, модифицирует r0
    out     USBOUT, x1      ;[10] <-- вывод
    rjmp    didStuffN       ;[0]

#define bitStatus   x3

sendNakAndReti:
    ldi     cnt, USBPID_NAK ;[-19]
    rjmp    sendCntAndReti  ;[-18]
sendAckAndReti:
    ldi     cnt, USBPID_ACK ;[-17]
sendCntAndReti:
    mov     r0, cnt         ;[-16]
    ldi     YL, 0           ;[-15] R0 адрес равен 0
    ldi     YH, 0           ;[-14]
    ldi     cnt, 2          ;[-13]
;   rjmp    usbSendAndReti      проваливаемся

;usbSend:
;указатель на данные в 'Y'
;количество байт в 'cnt' -- включая байт sync [диапазон 2 ... 12]
;используются: x1...x4, shift, cnt, Y
;числа в скобках определяет время с тех пор, как первый бит синхронизирующего шаблона (sync pattern) послан
usbSendAndReti:             ; 12 циклов пока не появится SOP
    in      x2, USBDDR      ;[-12]
    ori     x2, USBMASK     ;[-11]
    sbi     USBOUT, USBMINUS;[-10] подготовка состояния ожидания (idle); D+ и D- должны быть в 0 (без нагрузочных резисторов (pullups))
    in      x1, USBOUT      ;[-8] зеркалирование порта для цикла передачи (tx)
    out     USBDDR, x2      ;[-7] <- получение шины
; не нужно инициализировать x2 (история bitstuff (вставки неинформационных бит)), поскольку sync стартует с 0
    ldi     x4, USBMASK     ;[-6] маска exor (исключающее ИЛИ)
    ldi     shift, 0x80     ;[-5] байт sync - первый из отправляемых байт
    ldi     bitStatus, 0xff ;[-4] инициализируем счетчик цикла/бит, работает для количества до 12 байт
byteloop:
bitloop:
    sbrs    shift, 0        ;[8] [-3]
    eor     x1, x4          ;[9] [-2]
    out     USBOUT, x1      ;[10] [-1] <-- вывод
    ror     shift           ;[0]
    ror     x2              ;[1]
didStuffN:
    cpi     x2, 0xfc        ;[2]
    brcc    bitstuffN       ;[3]
    nop                     ;[4]
    subi    bitStatus, 37   ;[5] 256 / 7 ~=~ 37
    brcc    bitloop         ;[6] когда мы покидаем цикл, bitStatus имеет почти начальное значение
    sbrs    shift, 0        ;[7]
    eor     x1, x4          ;[8]
    ror     shift           ;[9]
didStuff7:
    out     USBOUT, x1      ;[10] <-- вывод
    ror     x2              ;[0]
    cpi     x2, 0xfc        ;[1]
    brcc    bitstuff7       ;[2]
    ld      shift, y+       ;[3]
    dec     cnt             ;[5]
    brne    byteloop        ;[6]
;make SE0:
    cbr     x1, USBMASK     ;[7] подготовка SE0 [спецификация говорит, что EOP может быть от 21 до 25 циклов]
    lds     x2, usbNewDeviceAddr;[8]
    lsl     x2              ;[10] мы сравниваем с влево сдвинутым адресом
    out     USBOUT, x1      ;[11] <-- вывод SE0-- теперь от 2 bits==20 циклов пока шина в состоянии ожидания (idle)
;2006-03-06: перенесена передача нового адреса в usbDeviceAddr из кода C в ассемблер:
;установка адреса только после того, как пакет данных отправлен, не после рукопожатия (handshake)
    subi    YL, 2           ;[0] только назначает адрес на пакеты данных, нет ACK/NAK в r0
    sbci    YH, 0           ;[1]
    breq    skipAddrAssign  ;[2]
    sts     usbDeviceAddr, x2; если не пропущено: SE0 на один цикл длиннее
skipAddrAssign:
;конец передачи usbDeviceAddress
    ldi     x2, 1<<USB_INTR_PENDING_BIT;[4] int0 произошло во время TX -- очистка флага ожидания очереди
    USB_STORE_PENDING(x2)   ;[5]
    ori     x1, USBIDLE     ;[6]
    in      x2, USBDDR      ;[7]
    cbr     x2, USBMASK     ;[8] установка обоих ножек на ввод
    mov     x3, x1          ;[9]
    cbr     x3, USBMASK     ;[10] конфигурируем отсутствие нагрузочных резисторов (pullup) на обоих ножках
    ldi     x4, 4           ;[11]
se0Delay:
    dec     x4              ;[12] [15] [18] [21]
    brne    se0Delay        ;[13] [16] [19] [22]
    out     USBOUT, x1      ;[23] <-- вывод J (idle) -- окончание SE0 (сигнал EOP)
    out     USBDDR, x2      ;[24] <-- теперь освобождение шины
    out     USBOUT, x3      ;[25] <-- убедимся в том, что нет активных нагрузочных (pull-up) резисторов
    rjmp    doReturn
