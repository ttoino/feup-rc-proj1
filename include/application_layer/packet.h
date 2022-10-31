#ifndef _PACKET_H_
#define _PACKET_H_

#include <stdint.h>
#include <stdlib.h>

#include "byte_vector.h"
#include "link_layer.h"

#undef LOG_NAME
#define LOG_NAME "APPLICATION LAYER"

#ifndef PACKET_DATA_SIZE
#define PACKET_DATA_SIZE 1024
#endif

/**
 * @brief the size of a complete packet, including packet header and packet
 *        body.
 */
#define PACKET_SIZE PACKET_DATA_SIZE + 4

/**
 * @brief A DATA packet, containing a data fragment to be transmitted over the
 *        physical transmission medium.
 */
#define DATA_PACKET (uint8_t)1

/**
 * @brief a START packet, containing metadata about the file being transmitted.
 */
#define START_PACKET (uint8_t)2

/**
 * @brief an END packet, signalling the end of the transmission.
 */
#define END_PACKET (uint8_t)3

/**
 * @brief the FILE_SIZE field in a START packet.
 */
#define FILE_SIZE_FIELD (uint8_t)1

/**
 * @brief the FILE_NAME field in a START packet.
 */
#define FILE_NAME_FIELD (uint8_t)2

/**
 * @brief Create a START packet.
 *
 * @param file_size The size of the file to transmit.
 * @param file_name The name of the file to transmit.
 *
 * @return A ByteVector object containing the packet bytes.
 */
ByteVector *create_start_packet(size_t file_size, const char *file_name);

/**
 * @brief Create a DATA packet.
 *
 * @param buf The data to send.
 * @param size The size of the data to send.
 *
 * @return A ByteVector object containing the packet bytes.
 */
ByteVector *create_data_packet(const uint8_t *buf, uint16_t size);

/**
 * @brief Create an END packet.
 *
 * @return A ByteVector object containing the packet bytes.
 */
ByteVector *create_end_packet();

/**
 * @brief Sends the specified packet using the specified connection object.
 *
 * @param connection The connection to send data to.
 * @param packet The packet to send.
 *
 * @return The number of bytes sent.
 */
ssize_t send_packet(LLConnection *connection, ByteVector *packet);

#endif // _PACKET_H_
