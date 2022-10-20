#ifndef _PACKET_H_
#define _PACKET_H_

#include <stdint.h>
#include <stdlib.h>

#include "byte_vector.h"
#include "link_layer.h"

#define LOG_NAME "APPLICATION LAYER"

#define PACKET_DATA_SIZE 1024
#define PACKET_SIZE PACKET_DATA_SIZE + 4

#define DATA_PACKET (uint8_t)1
#define START_PACKET (uint8_t)2
#define END_PACKET (uint8_t)3

#define FILE_SIZE_FIELD (uint8_t)1
#define FILE_NAME_FIELD (uint8_t)2

ByteVector *create_start_packet(size_t file_size, const char *file_name);
ByteVector *create_data_packet(const uint8_t *buf, uint16_t size);
ByteVector *create_end_packet();

ssize_t send_packet(LLConnection *connection, ByteVector *packet);

#endif // _PACKET_H_
