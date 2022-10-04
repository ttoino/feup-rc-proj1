// Application layer protocol implementation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "application_layer.h"
#include "link_layer.h"

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename) {
    LinkLayer ll = {
        .baudRate = baudRate, .nRetransmissions = nTries, .timeout = timeout};

    strncpy(ll.serialPort, serialPort, sizeof(ll.serialPort) - 1);

    LinkLayerRole r = LlTx;
    if (strcmp(role, "rx") == 0)
        r = LlRx;

    ll.role = r;

    llopen(ll);

    if (ll.role == LlRx) {
        unsigned char *s = NULL;
        while (llread(s) != -1)
            printf("\"%s\"\n", s);
        free(s);
    } else {
        char *s = "Hello world!";

        while (llwrite(s, strlen(s)) != -1)
            ;
    }

    llclose(FALSE);
}
