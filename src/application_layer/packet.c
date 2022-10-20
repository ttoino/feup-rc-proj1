#include "application_layer/packet.h"
#include "byte_vector.h"

#include <string.h>
#include <unistd.h>

ByteVector *create_start_packet(size_t file_size, const char *file_name) {
    ByteVector *bv = bv_create();

    bv_pushb(bv, START_PACKET);

    bv_pushb(bv, FILE_SIZE_FIELD);
    size_t i = bv->length;
    bv_pushb(bv, 0);
    while (file_size > 0) {
        bv_pushb(bv, (uint8_t)(file_size & 0xFF));
        bv_set(bv, i, bv_get(bv, i) + 1);
        file_size = file_size >> 8;
    }

    bv_pushb(bv, FILE_NAME_FIELD);
    size_t file_name_size = strlen(file_name);
    if (file_name_size > 255)
        file_name_size = 255;
    bv_pushb(bv, file_name_size);
    bv_push(bv, (const uint8_t *)file_name, file_name_size);

    return bv;
}

ByteVector *create_data_packet(const uint8_t *buf, uint16_t size) {
    static uint8_t sequence_number = 0;

    ByteVector *bv = bv_create();

    bv_pushb(bv, DATA_PACKET);
    bv_pushb(bv, sequence_number++);
    bv_pushb(bv, (uint8_t)((size & 0xFF00) >> 8));
    bv_pushb(bv, (uint8_t)(size & 0xFF));
    bv_push(bv, buf, size);

    return bv;
}

ByteVector *create_end_packet() {
    ByteVector *bv = bv_create();

    bv_pushb(bv, END_PACKET);

    return bv;
}

ssize_t send_packet(LLConnection *connection, ByteVector *packet) {
    ssize_t result = llwrite(connection, packet->array, packet->length);
    bv_destroy(packet);
    return result;
}
