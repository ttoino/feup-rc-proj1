#include "application_layer/packet.h"
#include "byte_vector.h"

void packet_destroy(Packet* this) {

    if (this == NULL) return;

    bv_destroy(this->information);
    free(this);
}