// Based on mziwisky espmissingincludes.h && ESP8266_IoT_SDK_Programming Guide_v0.9.1.pdf && ESP SDK defines

#ifndef __ESP_SYSTEM_API_H__
#define __ESP_SYSTEM_API_H__

#include <ets_sys.h>
#include <osapi.h>
#include <gpio.h>
#include <os_type.h>
#include <user_interface.h>
#include <spi_flash.h>
#include <espconn.h>
#include "espinc/uart.h"
#include "espinc/uart_register.h"
#include "espinc/spi_register.h"

#include <stdarg.h>

#include <user_config.h>

#include "m_printf.h"
#include "FakePgmSpace.h"
#include "debug_progmem.h"



#define __ESP8266_EX__ // System definition ESP8266 SOC



#ifdef ENABLE_GDB
	#define GDB_IRAM_ATTR IRAM_ATTR
#else
	#define GDB_IRAM_ATTR
#endif

#undef assert
//#define debugf(fmt, ...) m_printf(fmt"\r\n", ##__VA_ARGS__)

#define debugf LOG_E

#define assert(condition) if (!(condition)) SYSTEM_ERROR("ASSERT: %s %d", __FUNCTION__, __LINE__)
#define SYSTEM_ERROR(fmt, ...) m_printf("ERROR: " fmt "\r\n", ##__VA_ARGS__)

extern void ets_timer_arm_new(ETSTimer *ptimer, uint32_t milliseconds, bool repeat_flag, int isMstimer);
extern void ets_timer_disarm(ETSTimer *a);
extern void ets_timer_setfn(ETSTimer *t, ETSTimerFunc *pfunction, void *parg);

void *ets_bzero(void *block, size_t size);
bool ets_post(uint32_t prio, ETSSignal sig, ETSParam par);
void ets_task(ETSTask task, uint32_t prio, ETSEvent * queue, uint8 qlen);

//extern void ets_wdt_init(uint32_t val); // signature?
extern void ets_wdt_enable(void);
extern void ets_wdt_disable(void);
extern void wdt_feed(void);
//extern void wd_reset_cnt(void);
extern void ets_delay_us(uint32_t us);

extern void ets_isr_mask(unsigned intr);
extern void ets_isr_unmask(unsigned intr);
extern void ets_isr_attach(int intr, void *handler, void *arg);

extern int ets_memcmp(const void *s1, const void *s2, size_t n);
extern void *ets_memcpy(void *dest, const void *src, size_t n);
extern void *ets_memset(void *s, int c, size_t n);

extern void ets_install_putc1(void *routine);
extern int ets_sprintf(char *str, const char *format, ...)  __attribute__ ((format (printf, 2, 3)));
extern int ets_str2macaddr(void *, void *);
extern int ets_strcmp(const char *s1, const char *s2);
extern char *ets_strcpy(char *dest, const char *src);
//extern int os_random();
//extern char *ets_strdup(const char *str); // :(
const char * ets_strrchr(const char *str, int character);
extern size_t ets_strlen(const char *s);
extern int ets_strncmp(const char *s1, const char *s2, int len);
extern char *ets_strncpy(char *dest, const char *src, size_t n);
extern char *ets_strstr(const char *haystack, const char *needle);
extern int os_printf_plus(const char *format, ...)  __attribute__ ((format (printf, 1, 2)));
extern int os_snprintf(char *str, size_t size, const char *format, ...) __attribute__ ((format (printf, 3, 4)));
extern int ets_vsnprintf(char * s, size_t n, const char * format, va_list arg) __attribute__ ((format (printf, 3, 0)));

void system_pp_recycle_rx_pkt(void *eb);


#ifndef MEMLEAK_DEBUG
	extern void *pvPortMalloc( size_t xWantedSize );
	extern void vPortFree( void *pv );
	extern void *pvPortZalloc(size_t size);

	extern void *pvPortCalloc(size_t size, size_t count);
	extern void *pvPortRealloc(void* ptr, size_t size);

	#define malloc pvPortMalloc
	#define realloc pvPortRealloc
	#define free vPortFree
	#define zalloc pvPortZalloc
	#define calloc pvPortCalloc

#else
	extern void *pvPortMalloc(size_t xWantedSize, const char *file, uint32 line);
	extern void *pvPortZalloc(size_t xWantedSize, const char *file, uint32 line);
	extern void vPortFree(void *ptr, const char *file, uint32 line);
	extern void *pvPortCalloc(size_t size, size_t count, const char *file, uint32 line);
	extern void *pvPortRealloc(void* ptr, size_t size, const char *file, uint32 line);

	/*#define malloc(x) pvPortMalloc(x, __FILE__, __LINE__)
	#define realloc(x,y) pvPortRealloc(x, y, __FILE__, __LINE__)
	#define free(x) pvPortFree(x, __FILE__, __LINE__)
	#define zalloc(x) pvPortZalloc(x, __FILE__, __LINE__)
	#define calloc(x,y) pvPortCalloc(x, y, __FILE__, __LINE__)*/

#endif /*MEMLEAK_DEBUG*/

#define pvPortFree vPortFree
#define vPortMalloc pvPortMalloc

extern size_t xPortGetFreeHeapSize(void);
extern void prvHeapInit(void) ICACHE_FLASH_ATTR ;

extern void uart_div_modify(int no, unsigned int freq);
extern int ets_uart_printf(const char *fmt, ...);
extern void uart_tx_one_char(char ch);

extern void ets_intr_lock();
extern void ets_intr_unlock();

// CPU Frequency
extern void ets_update_cpu_frequency(uint32_t frq);
extern uint32_t ets_get_cpu_frequency();

typedef signed short file_t;

#endif
