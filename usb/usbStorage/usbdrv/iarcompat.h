/* Имя: iarcompat.h
 * Проект: AVR USB driver
 * Автор: Christian Starkjohann
 * Перевод: microsin.ru 
 * Дата создания: 2006-03-01
 * Табуляция: 4
 * Copyright: (c) 2006 by OBJECTIVE DEVELOPMENT Software GmbH
 * Лицензия: GNU GPL v2 (см. License.txt) или проприетарная (CommercialLicense.txt)
 * Ревизия: $Id: iarcompat.h 533 2008-02-28 15:35:25Z cs $
 */

/*
Основное описание:
Этот заголовочный файл включается, когда мы компилируем с IAR C-компилятором и ассемблером.
Файл задает макрос для совместимости между gcc и IAR-cc.

Благодарим Олега Семенова за помощь в портировании на IAR!
*/

#ifndef __iarcompat_h_INCLUDED__
#define __iarcompat_h_INCLUDED__

#if defined __IAR_SYSTEMS_ICC__ || defined __IAR_SYSTEMS_ASM__

/* Разрешение задания битов */
#ifndef ENABLE_BIT_DEFINITIONS
#   define ENABLE_BIT_DEFINITIONS	1
#endif

/* Включение заголовочных файлов IAR */
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

/* Следующие определения не нужны для драйвера, однако могут помочь,
 * если Вы портируете на IAR проект, основанный на gcc.
 */
#define cli()       __disable_interrupt()
#define sei()       __enable_interrupt()
#define wdt_reset() __watchdog_reset()

/* В зависимости от используемого микроконтроллера (MCU) Вы можете получить
 *  проблемы из-за того, что хендлы usbdrv.h для каждого типа MCU отличаются. 
 *  Поскольку IAR не использует операторы #define для регистров MCU, мы не можем
 *  проверить наличие определенного регистра с помощью #ifdef. Если механизм 
 *  автоопределения не срабатывает, включите определения для нужного макроса
 *  USB_INTR_* в Ваш usbconfig.h. См. подробности в usbconfig-prototype.h и usbdrv.h.
 */

#endif  /* defined __IAR_SYSTEMS_ICC__ || defined __IAR_SYSTEMS_ASM__ */
#endif  /* __iarcompat_h_INCLUDED__ */
