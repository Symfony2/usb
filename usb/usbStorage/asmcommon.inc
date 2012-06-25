/* Имя: asmcommon.inc
 * Проект: драйвер AVR USB
 * Автор: Christian Starkjohann
 * Перевод: microsin.ru
 * Дата создания: 2007-11-05
 * Табуляция: 4
 * Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 * Лицензия: GNU GPL v2 (см. License.txt) или проприетарная (CommercialLicense.txt)
 * Ревизия: $Id$
 */

/* Не линкуйте этот файл! Вместо этого линкуйте usbdrvasm.S, который в включает
 * соответствующую реализацию кода!
 */

/*
Основное описание:
Этот файл содержит ассемблерный код, который является общим между реализациями драйвера USB
для различных тактовых частот CPU. Поскольку код должен вставляться в среднюю часть модуля,
он разделен на этот файл и включаемый (#included).

Переходы, вызываемые снаружи:
    sofError: Вызывается, когда найдена последовательность no start (или не найдена последовательность start?).
    se0:      Вызывается, когда пакет успешно принят.
    overflow: Вызывается, когда буфер приема переполнен.
    doReturn: Вызывается после отправки данных.

Переходы наружу из этого модуля:
    waitForJ: Вызывается для приема уже пришедшего пакета.
    sendAckAndReti:
    sendNakAndReti:
    sendCntAndReti:
    usbSendAndReti:

Следующий макрос должен быть задан перед тем, как этот файл должен быть подключен:
    .macro POP_STANDARD
    .endm
    .macro POP_RETI
    .endm
*/

#define token   x1

overflow:
    ldi     x2, 1<<USB_INTR_PENDING_BIT
    USB_STORE_PENDING(x2)       ; очистка любых ожидающих прерываний
ignorePacket:
    clr     token
    rjmp    storeTokenAndReturn

;----------------------------------------------------------------------------
; Обработка принятого пакета (числа в скобках указывают на количество циков после середины SE0)
;----------------------------------------------------------------------------
;Здесь только выход типа non-error для цикла программы приема.
;Мы не проверяем любые CRC, потому что для вычислений нет времени.
se0:
    subi    cnt, USB_BUFSIZE    ;[5]
    neg     cnt                 ;[6]
    sub     YL, cnt             ;[7]
    sbci    YH, 0               ;[8]
    ldi     x2, 1<<USB_INTR_PENDING_BIT ;[9]
    USB_STORE_PENDING(x2)       ;[10] очистка ожидаемоко прерывания и далее проверка флага. SE0 должен закончиться.

    ld      token, y            ;[11]
    cpi     token, USBPID_DATA0 ;[13]
    breq    handleData          ;[14]
    cpi     token, USBPID_DATA1 ;[15]
    breq    handleData          ;[16]
    lds     shift, usbDeviceAddr;[17]
    ldd     x2, y+1             ;[19] ADDR и 1 бит номера конечной точки (endpoint)
    lsl     x2                  ;[21] выдвинем 1 бит endpoint
    cpse    x2, shift           ;[22]
    rjmp    ignorePacket        ;[23]
/* вычисляем только номер endpoint в x3, если требуется далее */
#if USB_CFG_HAVE_INTRIN_ENDPOINT || USB_CFG_IMPLEMENT_FN_WRITEOUT
    ldd     x3, y+2             ;[24] номер endpoint + crc
    rol     x3                  ;[26] вдвинем младший бит (LSB) endpoint
#endif
    cpi     token, USBPID_IN    ;[27]
    breq    handleIn            ;[28]
    cpi     token, USBPID_SETUP ;[29]
    breq    handleSetupOrOut    ;[30]
    cpi     token, USBPID_OUT   ;[31]
    brne    ignorePacket        ;[32] должен быть ack, nak или что угодно
;   rjmp    handleSetupOrOut    ;прямой спад

;Настройка и вывод (далее идет пакет данных) двух бит времени (16 cycles) после
; окончания SE0. Код синхронизации позволяет до 40 циклов задержки от начала старта 
; маркера синхронизации до выборки 1-го бита. Всего 56 циклов.
handleSetupOrOut:               ;[32]
#if USB_CFG_IMPLEMENT_FN_WRITEOUT   /* если мы имеем данные для endpoint != 0, установите в usbCurrentTok адрес */
    andi    x3, 0xf             ;[32]
    breq    storeTokenAndReturn ;[33]
    mov     token, x3           ;[34] индикация, что это endpoint x OUT
#endif
storeTokenAndReturn:
    sts     usbCurrentTok, token;[35]
doReturn:
    POP_STANDARD                ;[37] 12...16 циклов
    USB_LOAD_PENDING(YL)        ;[49]
    sbrc    YL, USB_INTR_PENDING_BIT;[50] проверка, дошли ли данные
    rjmp    waitForJ            ;[51] сохранение значений pop и push -- ожидается новое прерывание
sofError:
    POP_RETI                    ;вызов макроса
    reti

handleData:
    lds     token, usbCurrentTok;[18]
    tst     token               ;[20]
    breq    doReturn            ;[21]
    lds     x2, usbRxLen        ;[22]
    tst     x2                  ;[24]
    brne    sendNakAndReti      ;[25]
; 2006-03-11: Следующие 2 строки исправляют проблему с тем, что устройство не распознается,
; если usbPoll() был вызван менее часто чем каждые 4 ms.
    cpi     cnt, 4              ;[26] пакеты с нулевым размером тольк фаза статуса -- игнорировать и ack
    brmi    sendAckAndReti      ;[27] сохраняем rx буфер чистым -- мы не должны делать NAK в следующий SETUP
    sts     usbRxLen, cnt       ;[28] сохраняем принятые данные, меняем буферА
    sts     usbRxToken, token   ;[30]
    lds     x2, usbInputBufOffset;[32] меняем буферА
    ldi     cnt, USB_BUFSIZE    ;[34]
    sub     cnt, x2             ;[35]
    sts     usbInputBufOffset, cnt;[36] буферА поменяны местами
    rjmp    sendAckAndReti      ;[38] 40 + 17 = 57 до момента SOP

handleIn:
;Мы не отправляем какие бы то ни было данные пока код C не обработает текущие входные данные
; и потенциально обновленные выходные данные. Это более эффективно в смысле размера кода,
; чем очистка буферов tx когда пакет принят.
    lds     x1, usbRxLen        ;[30]
    cpi     x1, 1               ;[32] отрицательные величины для управления потоком (flow control), 0 означает "буфер пуст"
    brge    sendNakAndReti      ;[33] необработанный входной пакет?
    ldi     x1, USBPID_NAK      ;[34] подготовим величину для usbTxLen
#if USB_CFG_HAVE_INTRIN_ENDPOINT
    andi    x3, 0xf             ;[35] x3 содержит endpoint
    brne    handleIn1           ;[36]
#endif
    lds     cnt, usbTxLen       ;[37]
    sbrc    cnt, 4              ;[39] все символы handshake имеют набор из 4 бит
    rjmp    sendCntAndReti      ;[40] 42 + 16 = 58 до появления SOP
    sts     usbTxLen, x1        ;[41] x1 == USBPID_NAK от кода ранее
    ldi     YL, lo8(usbTxBuf)   ;[43]
    ldi     YH, hi8(usbTxBuf)   ;[44]
    rjmp    usbSendAndReti      ;[45] 57 + 12 = 59 до появления SOP

;Пояснения, как устанавливать usbTxLen для USBPID_NAK:
; Мы должны установить его назад когда мы приняли ACK от хоста. Это легко реализовать:
; одна статическая переменная, которая сохраняет последнее tx для endpoint 0 и 1 и
; сравнить в приемнике для различения ACK. Однако мы устанвим его обратно сразу когда
; мы отправляем пакет принимая, что ошибок нет и ност отправляет ACK. Мы сохраняем 
; этим способом 1 байт RAM и одновременно избегаем потенциальных проблем с бесконечными
; попытками повторов. Все равно драйвер предполагает передачи без ошибок.

#if USB_CFG_HAVE_INTRIN_ENDPOINT    /* это размещено здесь для относительных переходов */
handleIn1:                      ;[38]
#if USB_CFG_HAVE_INTRIN_ENDPOINT3
; 2006-06-10 как предложил O.Tamura: поддержка второй INTR IN / BULK IN endpoint
    cpi     x3, USB_CFG_EP3_NUMBER;[38]
    breq    handleIn3           ;[39]
#endif
    lds     cnt, usbTxLen1      ;[40]
    sbrc    cnt, 4              ;[42] все символы handshake имеют набор из 4 бит
    rjmp    sendCntAndReti      ;[43] 47 + 16 = 63 до появления SOP
    sts     usbTxLen1, x1       ;[44] x1 == USBPID_NAK от кода ранее
    ldi     YL, lo8(usbTxBuf1)  ;[46]
    ldi     YH, hi8(usbTxBuf1)  ;[47]
    rjmp    usbSendAndReti      ;[48] 50 + 12 = 62 до появления SOP
#endif

#if USB_CFG_HAVE_INTRIN_ENDPOINT && USB_CFG_HAVE_INTRIN_ENDPOINT3
handleIn3:
    lds     cnt, usbTxLen3      ;[41]
    sbrc    cnt, 4              ;[43]
    rjmp    sendCntAndReti      ;[44] 49 + 16 = 65 до появления SOP
    sts     usbTxLen3, x1       ;[45] x1 == USBPID_NAK от кода ранее
    ldi     YL, lo8(usbTxBuf3)  ;[47]
    ldi     YH, hi8(usbTxBuf3)  ;[48]
    rjmp    usbSendAndReti      ;[49] 51 + 12 = 63 до появления SOP
#endif
