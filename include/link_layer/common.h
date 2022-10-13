#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdlib.h>

#define TRUE 1
#define FALSE 0

#define BIT_B(b, n) ((b) << (n))
#define BIT(n) BIT_B(1, n)

unsigned char make_BCC(unsigned char addr, unsigned char cmd);

#endif // _COMMON_H_
