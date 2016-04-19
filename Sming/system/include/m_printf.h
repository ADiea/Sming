/*
Author: (github.com/)ADiea
Project: Sming for ESP8266 - https://github.com/SmingHub/Sming
License: MIT
Date: 21.12.2015
Descr: embedded very simple version of printf with float support
*/
#ifndef _M_PRINTF_
#define _M_PRINTF_

#include <stdarg.h>

int m_vsnprintf(char *buf, size_t maxLen, const char *fmt, va_list args);
int m_snprintf(char* buf, int length, const char *fmt, ...);
int m_printf(const char* fmt, ...);
int m_vprintf ( const char * format, va_list arg );

#endif /*_M_PRINTF_*/
