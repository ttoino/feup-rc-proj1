#ifndef _FRAME_H_
#define _FRAME_H_

/**
 * @brief A struct representing a frame.
 */
typedef struct _Frame Frame;

#include "byte_vector.h"
#include "link_layer.h"
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief Sets the n-th bit to b.
 */
#define BIT_B(b, n) ((b) << (n))
/**
 * @brief Sets the n-th bit to 1.
 */
#define BIT(n) BIT_B(1, n)

/**
 * @brief The receiver's address.
 *
 * Used on commands sent by the receiver and responses sent by the transmitter.
 */
#define RX_ADDR (uint8_t)0x03
/**
 * @brief The transmitter's address.
 *
 * Used on commands sent by the transmitter and responses sent by the receiver.
 */
#define TX_ADDR (uint8_t)0x07

/**
 * @brief A set-up command.
 */
#define SET (uint8_t)0b0011
/**
 * @brief A disconnect command.
 */
#define DISC (uint8_t)0b1011
/**
 * @brief An information command.
 *
 * Frames with this command have additional information.
 */
#define I(s) (uint8_t)(BIT_B((s), 6) | 0b0000)
/**
 * @brief Checks if a frame type is a command.
 */
#define IS_COMMAND(c) ((c) == SET || (c) == DISC || ((c)&0xf) == I(0))

/**
 * @brief An unnumbered acknowledgement response.
 *
 * Used as a response to #SET and #DISC.
 */
#define UA (uint8_t)0b0111
/**
 * @brief A receiver ready response.
 *
 * Used as a response to #I when no error was detected.
 */
#define RR(r) (uint8_t)(BIT_B((r), 7) | 0b0101)
/**
 * @brief A rejection response.
 *
 * Used as a response to #I when an error was detected.
 */
#define REJ(r) (uint8_t)(BIT_B((r), 7) | 0b0001)
/**
 * @brief Checks if a frame type is a response.
 */
#define IS_RESPONSE(c) ((c) == UA || ((c)&0xf) == RR(0) || ((c)&0xf) == REJ(0))

/**
 * @brief An information error command.
 *
 * Frames with this command had an error in their body.
 */
#define I_ERR (uint8_t)0b1111

/**
 * @brief The byte used to signal the beginning and ending of a frame.
 */
#define FLAG (uint8_t)0x7e
/**
 * @brief The byte used to escape special bytes.
 */
#define ESC (uint8_t)0x7d

/**
 * @brief An escaped #FLAG.
 */
#define ESC_FLAG (uint8_t)0x5e
/**
 * @brief An escaped #ESC.
 */
#define ESC_ESC (uint8_t)0x5d

/**
 * @brief A struct representing a frame.
 */
struct _Frame {
    /**
     * @brief This frame's address.
     *
     * Can be either #RX_ADDR or #TX_ADDR.
     */
    uint8_t address;

    /**
     * @brief This frame's command.
     *
     * Can be one of #UA, #SET, #DISC, #I, #RR, #REJ, or #I_ERR.
     */
    uint8_t command;

    /**
     * @brief Additional information sent with this frame.
     *
     * Only sent on #I frames.
     */
    ByteVector *information;
};

/**
 * @brief Creates a frame.
 *
 * @param connection The connection that's creating the frame.
 * @param cmd The command byte.
 *
 * @return The newly created frame.
 */
Frame *create_frame(LLConnection *connection, uint8_t cmd);

/**
 * @brief Reads a frame from a connection.
 *
 * @param connection The connection to read from.
 *
 * @return The frame that was read.
 */
Frame *read_frame(LLConnection *connection);

/**
 * @brief Writes a frame onto a connection.
 *
 * @param connection The connection to write to.
 *
 * @return The number of bytes written.
 * @return -1 on error.
 */
ssize_t write_frame(LLConnection *connection, Frame *frame);

/**
 * @brief Deallocates resources created by a frame.
 *
 * @param this The frame to deallocate.
 */
void frame_destroy(Frame *this);

/**
 * @brief Sends a frame through a connection.
 *
 * @note Equivalent to #write_frame for responses, automatically retransmits
 *       commands.
 *
 * @param connection The connection to send through.
 * @param frame The frame to be sent.
 *
 * @return The number of bytes written.
 * @return -1 on error.
 */
ssize_t send_frame(LLConnection *connection, Frame *frame);

/**
 * @brief Reads frames continuously until a specified type is received.
 *
 * @param connection The connection to read from.
 * @param command The command byte to expect.
 *
 * @return The frame that was read.
 */
Frame *expect_frame(LLConnection *connection, uint8_t command);

/**
 * @brief Construct the textual representation of the given command.
 *
 * @param command The command to convert into a human-readable string.
 *
 * @return The human-readable string representation of the given command.
 */
char *get_command(uint8_t command);

#endif // _FRAME_H_
