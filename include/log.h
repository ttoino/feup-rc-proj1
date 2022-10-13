#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>

#define _LOG(prefix, ...) printf("[" prefix "]: " __VA_ARGS__)
#define LOG(...) _LOG("LOG", __VA_ARGS__)
#define INFO(...) _LOG("INFO", __VA_ARGS__)
#define ALARM(...) _LOG("ALARM", __VA_ARGS__)
#define ERROR(...) _LOG("ERROR", __VA_ARGS__)

#endif // _LOG_H_
