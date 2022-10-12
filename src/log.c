#include <stdio.h>
#include <stdarg.h>

#include "log.h"

void LOG(const char *format, ...) {

    va_list args;
    va_start(args, format);

    printf("[LOG]: ");    
    vprintf(format, args);

    va_end(args);

}

void INFO(const char *format, ...) {
    va_list args;
    va_start(args, format);

    printf("[INFO]: ");    
    vprintf(format, args);

    va_end(args);
}

void ALARM(const char *format, ...) {
    va_list args;
    va_start(args, format);

    printf("[ALARM]: ");    
    vprintf(format, args);

    va_end(args);
}

void ERROR(const char *format, ...) {
    va_list args;
    va_start(args, format);

    printf("[ERROR]: ");    
    vprintf(format, args);

    va_end(args);
}
