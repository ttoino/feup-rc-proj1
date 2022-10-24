#include "link_layer/frame.h"
#include "link_layer.h"
#include "link_layer/timer.h"
#include "log.h"

#include <stdbool.h>
#include <string.h>
#include <unistd.h>

/**
 * @brief An enum representing the valid states in the state machine for reading
 *        a frame.
 */
typedef enum {
    /**
     * @brief The starting state.
     */
    START,
    /**
     * @brief The state after the first #FLAG was read.
     */
    FLAG_RCV,
    /**
     * @brief The state after the address was read.
     */
    A_RCV,
    /**
     * @brief The state after the command was read.
     */
    C_RCV,
    /**
     * @brief The state after the bcc was read and verified.
     */
    BCC_RCV,
    /**
     * @brief The state after some data in an I frame was read.
     */
    DATA_RCV,
    /**
     * @brief The state after an #ESC byte was read.
     */
    ESC_RCV,
    /**
     * @brief The state after the last #FLAG was received.
     */
    END_FLAG_RCV,
    /**
     * @brief The last state, means the state machine accepted the frame.
     */
    END,
    /**
     * @brief The error state, means there was an error in the body of an I
     *        frame.
     */
    NACK,
} ReadFrameState;

Frame *create_frame(LLConnection *connection, uint8_t cmd) {
    Frame *frame = malloc(sizeof(Frame));

    if (frame == NULL)
        return NULL;

    frame->address =
        (IS_COMMAND(cmd) && connection->params.role == LL_RX) ||
                (IS_RESPONSE(cmd) && connection->params.role == LL_TX)
            ? RX_ADDR
            : TX_ADDR;
    frame->command = cmd;

    return frame;
}

uint8_t make_bcc(Frame *frame) {
    return (uint8_t)(frame->address ^ frame->command);
}

/**
 * @brief Checks if a frame's address and command combination can be handled by
 *        this layer's role.
 *
 * @param frame The frame.
 * @param role This layer's role
 *
 * @return Whether the frame's address and command combination is legal.
 */
bool check_command_and_address(Frame *frame, LLRole role) {
    return (IS_COMMAND(frame->command) &&
            ((role == LL_RX && frame->address == TX_ADDR) ||
             (role == LL_TX && frame->address == RX_ADDR))) ||
           (IS_RESPONSE(frame->command) &&
            ((role == LL_RX && frame->address == RX_ADDR) ||
             (role == LL_TX && frame->address == TX_ADDR)));
}

Frame *read_frame(LLConnection *connection) {
    ReadFrameState state = START;
    Frame *frame = malloc(sizeof(Frame));
    frame->information = NULL;

    if (frame == NULL)
        return NULL;

    uint8_t temp;

    while (true) {
        switch (state) {
        case START:
            if (read(connection->fd, &temp, 1) != 1) {
                frame_destroy(frame);
                return NULL;
            }

            if (temp == FLAG)
                state = FLAG_RCV;

            break;

        case FLAG_RCV:
            if (read(connection->fd, &frame->address, 1) != 1) {
                frame_destroy(frame);
                return NULL;
            }

            if (frame->address == RX_ADDR || frame->address == TX_ADDR)
                state = A_RCV;
            else if (frame->address == FLAG)
                state = FLAG_RCV;
            else
                state = START;

            break;

        case A_RCV:
            if (read(connection->fd, &frame->command, 1) != 1) {
                frame_destroy(frame);
                return NULL;
            }

            if (check_command_and_address(frame, connection->params.role))
                state = C_RCV;
            else if (frame->command == FLAG)
                state = FLAG_RCV;
            else
                state = START;

            break;

        case C_RCV:
            if (read(connection->fd, &temp, 1) != 1) {
                frame_destroy(frame);
                return NULL;
            }

            if (temp == make_bcc(frame))
                state = BCC_RCV;
            else if (temp == FLAG)
                state = FLAG_RCV;
            else
                state = START;

            break;

        case BCC_RCV:
            if ((frame->command & 0xF) == I(0)) {
                state = DATA_RCV;
            } else {
                if (read(connection->fd, &temp, 1) != 1) {
                    frame_destroy(frame);
                    return NULL;
                }

                if (temp == FLAG)
                    state = END;
                else
                    state = START;
            }
            break;

        case DATA_RCV:
            if (frame->information == NULL)
                frame->information = bv_create();

            if (read(connection->fd, &temp, 1) != 1) {
                frame_destroy(frame);
                return NULL;
            }

            if (temp == ESC)
                state = ESC_RCV;
            else if (temp == FLAG)
                state = END_FLAG_RCV;
            else
                bv_pushb(frame->information, temp);

            break;

        case ESC_RCV:
            if (read(connection->fd, &temp, 1) != 1) {
                frame_destroy(frame);
                return NULL;
            }

            state = DATA_RCV;

            if (temp == ESC_ESC)
                bv_pushb(frame->information, ESC);
            else if (temp == ESC_FLAG)
                bv_pushb(frame->information, FLAG);
            else
                state = NACK;

            break;

        case NACK:
            frame->command |= I_ERR;
            state = END;
            break;

        case END_FLAG_RCV: {
            uint8_t expected_bcc2 = 0;
            uint8_t received_bcc2 = bv_popb(frame->information);

            for (size_t j = 0; j <= frame->information->length; ++j)
                expected_bcc2 ^= bv_get(frame->information, j);

            if (expected_bcc2 != received_bcc2)
                state = NACK;
            else
                state = END;
            break;
        }

        default:
            return frame;
        }
    }
}

/**
 * @brief Writes the byte stuffed information from an I frame into a buffer.
 *
 * @param buf Where to write the information.
 * @param frame The frame.
 */
void write_info(ByteVector *buf, Frame *frame) {
    uint8_t bcc = 0, temp;

    for (size_t i = 0; i < frame->information->length; ++i) {
        temp = bv_get(frame->information, i);
        bcc ^= temp;

        if (temp == FLAG) {
            bv_pushb(buf, ESC);
            bv_pushb(buf, ESC_FLAG);
        } else if (temp == ESC) {
            bv_pushb(buf, ESC);
            bv_pushb(buf, ESC_ESC);
        } else {
            bv_pushb(buf, temp);
        }
    }

    if (bcc == FLAG) {
        bv_pushb(buf, ESC);
        bv_pushb(buf, ESC_FLAG);
    } else if (bcc == ESC) {
        bv_pushb(buf, ESC);
        bv_pushb(buf, ESC_ESC);
    } else {
        bv_pushb(buf, bcc);
    }
}

ssize_t write_frame(LLConnection *connection, Frame *frame) {
    if (frame == NULL)
        return -1;

    ByteVector *buf = bv_create();

    bv_pushb(buf, FLAG);
    bv_pushb(buf, frame->address);
    bv_pushb(buf, frame->command);
    bv_pushb(buf, make_bcc(frame));

    if ((frame->command & 0xF) == I(0) && frame->information != NULL)
        write_info(buf, frame);

    bv_pushb(buf, FLAG);

    int bytes_written = write(connection->fd, buf->array, buf->length);

    bv_destroy(buf);

    return bytes_written;
}

void frame_destroy(Frame *this) {
    if (this == NULL)
        return;

    if (this->information != NULL)
        bv_destroy(this->information);

    free(this);
}

ssize_t send_frame(LLConnection *connection, Frame *frame) {
    ssize_t bytes_written = write_frame(connection, frame);

    if (bytes_written <= 0)
        return -1;

    if (IS_COMMAND(frame->command)) {
        frame_destroy(connection->last_command_frame);
        connection->last_command_frame = frame;
        connection->n_retransmissions_sent = 0;

        if (timer_arm(connection) == -1)
            return -1;
    } else {
        frame_destroy(frame);
    }

    return bytes_written;
}

ssize_t handle_frame(LLConnection *connection, Frame *frame) {
    LOG("Received frame (c = %s, 0x%02x)\n", get_command(frame->command),
        frame->command);

    switch (frame->command) {
    case SET:
        LOG("Sending UA frame to complete handshake!\n");
        return send_frame(connection, create_frame(connection, UA));

    case DISC:
        connection->closed = true;
        if (connection->params.role == LL_RX) {
            if (send_frame(connection, create_frame(connection, DISC)) == -1)
                return -1;

            Frame *f = expect_frame(connection, UA);
            if (f == NULL)
                return -1;

            frame_destroy(f);
        } else {
            LOG("Sending UA frame to complete disconnect phase!\n");
            return send_frame(connection, create_frame(connection, UA));
        }
        break;

    case I(0):
    case I(1): {
        uint8_t s = frame->command >> 6;
        return send_frame(connection, create_frame(connection, RR(1 - s)));
    }

    case UA:
    case RR(0):
    case RR(1):
        return timer_disarm(connection);

    case REJ(0):
    case REJ(1):
        return timer_force(connection);

    case I_ERR | I(0):
    case I_ERR | I(1): {
        uint8_t s = frame->command >> 6;
        return send_frame(connection, create_frame(connection, REJ(s)));
    }
    }

    return 0;
}

Frame *expect_frame(LLConnection *connection, uint8_t command) {
    Frame *frame;
    while (1) {
        frame = read_frame(connection);

        if (frame == NULL)
            return NULL;

        if (handle_frame(connection, frame) < 0) {
            frame_destroy(frame);
            return NULL;
        }

        if (frame->command == command)
            break;

        frame_destroy(frame);
    }

    return frame;
}

char *get_command(uint8_t command) {
    switch (command) {
    case SET:
        return "SET";
    case DISC:
        return "DISC";
    case I(0):
        return "I(0)";
    case I(1):
        return "I(1)";
    case UA:
        return "UA";
    case RR(0):
        return "RR(0)";
    case RR(1):
        return "RR(1)";
    case REJ(0):
        return "REJ(0)";
    case REJ(1):
        return "REJ(1)";
    case I(0) | I_ERR:
        return "I_ERR(0)";
    case I(1) | I_ERR:
        return "I_ERR(1)";
    }

    return "INVALID";
}
