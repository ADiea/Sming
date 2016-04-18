#ifndef DEBUG_PROGMEM_H
#define DEBUG_PROGMEM_H

//#include <SmingCore/SmingCore.h>
#include "FakePgmSpace.h"

#define DEBUG_BUILD 1

#define ERR 0	
#define WARN 1
#define INFO 2
#define DBG 3

#if DEBUG_BUILD
	#define VERBOSE_LEVEL INFO

	#define LOG_E(fmt, ...) \
		({const char _fmt[] __attribute__((aligned(4))) __attribute__((section(".irom.text"))) = fmt; \
		printf_P(_fmt, ##__VA_ARGS__);})
		
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