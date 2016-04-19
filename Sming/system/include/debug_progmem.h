#ifndef DEBUG_PROGMEM_H
#define DEBUG_PROGMEM_H

#include "FakePgmSpace.h"

#define DEBUG_BUILD 1

#define ERR 0	
#define WARN 1
#define INFO 2
#define DBG 3

#if DEBUG_BUILD
#define VERBOSE_LEVEL INFO

#define TOKEN_PASTE(x,y,z) x##y##z
#define TOKEN_PASTE2(x,y, z) TOKEN_PASTE(x,y,z)

#define ATTR_PASTE(x,y) MAKE_STRING(x##y)
#define ATTR_PASTE2(x,y) ATTR_PASTE(x,y)

#define MAKE_STRING(x) #x

#define READONLY_SECTION .irom.

//#define XSTR(x) S_TR(x)
//#define S_TR(x) #x
//#pragma message "File name is: " XSTR(CUST_FILE_BASE)

//This has to be one liner for the _log##TOKEN_PASTE2(__FUNCTION__, __LINE__) to work !!!
//({static const char TOKEN_PASTE2(log, __COUNTER__)[] PROGMEM = fmt; printf_P(TOKEN_PASTE3(log, __COUNTER__), ##__VA_ARGS__);})
#define LOG_E(fmt, ...) \
	({static const char TOKEN_PASTE2(log_,CUST_FILE_BASE,__LINE__)[] \
	__attribute__((aligned(4))) \
	__attribute__((section(ATTR_PASTE2(READONLY_SECTION,CUST_FILE_BASE)))) = fmt; \
	printf_P(TOKEN_PASTE2(log_,CUST_FILE_BASE,__LINE__), ##__VA_ARGS__);})
		
	/*
		#define LOG_E(fmt, ...) m_printf(_fmt"\n", ##__VA_ARGS__)
	*/	
		
//#define LOG_E(fmt, ...)m_printf("tag"##__FUNCTION__##fmt, ##__VA_ARGS__)

	#if VERBOSE_LEVEL == DBG
		#define LOG_W LOG_E
		#define LOG_I LOG_E
		#define LOG_D LOG_E
	#elif VERBOSE_LEVEL == INFO
		#define LOG_W LOG_E
		#define LOG_I LOG_E
		#define LOG_D
	#elif VERBOSE_LEVEL == WARN
		#define LOG_W LOG_E
		#define LOG_I
		#define LOG_D
	#elif VERBOSE_LEVEL == ERR
		#define LOG_W
		#define LOG_I
		#define LOG_D
	#else
		#error Set debug verbose level in debug.h
	#endif
#else
	#define LOG_E
	#define LOG_W
	#define LOG_I
	#define LOG_D
#endif

#endif /*#ifndef DEBUG_PROGMEM_H*/
