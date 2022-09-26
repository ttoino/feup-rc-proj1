// Application layer protocol implementation

#include <string.h>

#include "application_layer.h"
#include "link_layer.h"

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer ll = {
	.baudRate = baudRate,
	.nRetransmissions = nTries,
	.timeout = timeout
    };

    strncpy(ll.serialPort, serialPort, sizeof(ll.serialPort) - 1);

    LinkLayerRole r = LlTx;
    if (strcmp(role, "rx") == 0) r = LlRx;

    ll.role = r;

    llopen(ll);


    llclose(FALSE);
}
