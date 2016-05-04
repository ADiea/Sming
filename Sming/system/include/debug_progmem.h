#ifndef DEBUG_PROGMEM_H
#define DEBUG_PROGMEM_H

#include <stdarg.h>
#include "FakePgmSpace.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG_BUILD 1

#define ERR 0	
#define WARN 1
#define INFO 2
#define DBG 3

#if DEBUG_BUILD

#define VERBOSE_LEVEL INFO

#define TOKEN_PASTE(x,y,z) x##y##z
#define TOKEN_PASTE2(x,y, z) TOKEN_PASTE(x,y,z)

#define GET_FILENAME(x) #x
#define GET_FNAME2(x) GET_FILENAME(x)

#define LOG_E(fmt, ...) \
	({static const char TOKEN_PASTE2(log_,CUST_FILE_BASE,__LINE__)[] \
	__attribute__((aligned(4))) \
	__attribute__((section(".irom.text"))) = fmt " \t[" GET_FNAME2(CUST_FILE_BASE) ":%d]\n"; \
	printf_P(TOKEN_PASTE2(log_,CUST_FILE_BASE,__LINE__), ##__VA_ARGS__, __LINE__);})

//log inline with no \n
#define LOG_EI(fmt, ...) \
	({static const char TOKEN_PASTE2(log_,CUST_FILE_BASE,__LINE__)[] \
	__attribute__((aligned(4))) \
	__attribute__((section(".irom.text"))) = fmt; \
	printf_P(TOKEN_PASTE2(log_,CUST_FILE_BASE,__LINE__), ##__VA_ARGS__);})
		

	#if VERBOSE_LEVEL == DBG
		#define LOG_W LOG_E
		#define LOG_WI LOG_EI
		#define LOG_I LOG_E
		#define LOG_II LOG_EI
		#define LOG_D LOG_E
		#define LOG_DI LOG_EI
	#elif VERBOSE_LEVEL == INFO
		#define LOG_W LOG_E
		#define LOG_WI LOG_EI
		#define LOG_I LOG_E
		#define LOG_II LOG_EI
		#define LOG_D
		#define LOG_DI
	#elif VERBOSE_LEVEL == WARN
		#define LOG_W LOG_E
		#define LOG_WI LOG_EI
		#define LOG_I
		#define LOG_II
		#define LOG_D
		#define LOG_DI
	#else
		#define LOG_W
		#define LOG_WI
		#define LOG_I
		#define LOG_II
		#define LOG_D
		#define LOG_DI
	#endif
#else
	#define LOG_W
	#define LOG_WI
	#define LOG_I
	#define LOG_II
	#define LOG_D
	#define LOG_DI
#endif /*DEBUG_BUILD*/

#ifdef __cplusplus
}
#endif

#endif /*#ifndef DEBUG_PROGMEM_H*/
