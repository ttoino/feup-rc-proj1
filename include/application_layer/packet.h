#ifndef _PACKET_H_
#define _PACKET_H_

#include <stdint.h>
#include <stdlib.h>

#include "byte_vector.h"
#include "link_layer.h"

/**
 * @brief Represents the type of packet that is being sent
 * 
 */
enum packet_type {
    DATA = 1,
    START,
    END
};

/**
 * @brief Helper enum for 
 * 
 */
enum control_packet_field_type {
    FILE_SIZE,
    FILE_NAME
};

/**
 * @brief A struct representing a packet.
 */
typedef struct {

    /**
     * @brief the type of this packet.
     * 
     * This is separated for ease of use.
     */
    enum packet_type type;

    /**
     * @brief Packet data. Contains all the bytes related to this packet, excluding the packet type.
     *
     * Can be null, see END control packets.
     */
    ByteVector *information;
} Packet;

/**
 * @brief Deallocates resources used for a packet.
 * 
 * @param this the packet to deallocate 
 */
void packet_destroy(Packet* this);

#endif // _PACKET_H_
