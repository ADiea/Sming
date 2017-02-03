/*
 * debug_progmem.h
 *
 *  Contains debug functions that facilitate using strings stored in flash(irom). 
 *  This frees up RAM of all const char* debug strings 
 *
 *  Created on: 27.01.2017
 *  Author: (github.com/)ADiea
 */
#ifndef DEBUG_PROGMEM_H
#define DEBUG_PROGMEM_H

#include <stdarg.h>
#include "FakePgmSpace.h"

#ifdef __cplusplus
extern "C" {
#endif

//This enables or disables logging
//Can be overridden in Makefile
#ifndef DEBUG_BUILD
	#ifdef SMING_RELEASE
		#define DEBUG_BUILD 0
	#else
		#define DEBUG_BUILD 1
	#endif
#endif

//This enables or disables file and number printing for each log line
//Can be overridden in Makefile
#ifndef PRINT_FILENAME_AND_LINE
#define PRINT_FILENAME_AND_LINE 0
#endif

#define ERR 0	
#define WARN 1
#define INFO 2
#define DBG 3

//This sets debug verbose level
//Define in Makefile, default is INFO
#ifndef DEBUG_VERBOSE_LEVEL
#define DEBUG_VERBOSE_LEVEL INFO
#endif

#if DEBUG_BUILD

#define TOKEN_PASTE(x,y,z) x##y##z
#define TOKEN_PASTE2(x,y, z) TOKEN_PASTE(x,y,z)

#define GET_FILENAME(x) #x
#define GET_FNAME2(x) GET_FILENAME(x)

#define CAT2(x,y) x##y
#define CONCAT(x,y) CAT2(x,y)
#define QUOT(x) #x
#define QUOTE(x) QUOT(x)
#define SET_SECT()
//
/*__attribute__((section(".irom.text." GET_FNAME2(__LINE__))))*/
/*__attribute__((section(".irom.text")))*/

//A static const char[] is defined having a unique name (log_ prefix, filename and line number)
//This will be stored in the irom section(on flash) feeing up the RAM
//Next special version of printf from FakePgmSpace is called to fetch and print the message
#if PRINT_FILENAME_AND_LINE
#define LOG_E(fmt, ...) \
	({static const char TOKEN_PASTE2(log_,CUST_FILE_BASE,__LINE__)[] \
	__attribute__((aligned(4))) \
	__attribute__((section(QUOTE(.irom.text)))) = "[" GET_FNAME2(CUST_FILE_BASE) ":%d] " fmt; \
	printf_P_stack(TOKEN_PASTE2(log_,CUST_FILE_BASE,__LINE__), __LINE__, ##__VA_ARGS__);})
#else
#define LOG_E(fmt, ...) \
	({static const char TOKEN_PASTE2(log_,CUST_FILE_BASE,__LINE__)[] \
	__attribute__((aligned(4))) \
	__attribute__((section(".irom.text"))) = fmt; \
	printf_P_stack(TOKEN_PASTE2(log_,CUST_FILE_BASE,__LINE__), ##__VA_ARGS__);})
#endif

	#if DEBUG_VERBOSE_LEVEL == DBG
		#define LOG_W LOG_E
		#define LOG_I LOG_E
		#define LOG_D LOG_E
	#elif DEBUG_VERBOSE_LEVEL == INFO
		#define LOG_W LOG_E
		#define LOG_I LOG_E
		#define LOG_D
	#elif DEBUG_VERBOSE_LEVEL == WARN
		#define LOG_W LOG_E
		#define LOG_I
		#define LOG_D
	#else
		#define LOG_W
		#define LOG_I
		#define LOG_D
	#endif
#else /*DEBUG_BUILD*/
	#define LOG_W
	#define LOG_I
	#define LOG_D
#endif /*DEBUG_BUILD*/

#ifdef __cplusplus
}
#endif

#endif /*#ifndef DEBUG_PROGMEM_H*/
