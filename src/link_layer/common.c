#include "link_layer/common.h"

unsigned char make_BCC(unsigned char addr, unsigned char cmd) {
    return (unsigned char)(addr ^ cmd);
}