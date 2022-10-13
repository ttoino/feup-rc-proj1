#include "link_layer.h"
#include "link_layer/common.h"
#include "link_layer/frame.h"

#include <stdbool.h>
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

bool check_command_and_address(Frame *frame, LinkLayerRole role) {
    return (IS_COMMAND(frame->command) &&
            ((role == LlRx && frame->address == TX_ADDR) ||
             (role == LlTx && frame->address == RX_ADDR))) ||
           (IS_RESPONSE(frame->command) &&
            ((role == LlRx && frame->address == RX_ADDR) ||
             (role == LlTx && frame->address == TX_ADDR)));
}

Frame *read_frame(int fd, LinkLayerRole role) {
    ReadFrameState state = START;
    Frame *frame = malloc(sizeof(Frame));
    frame->information = NULL;
    uint8_t temp;

    while (TRUE) {

        switch (state) {
        case START:
            read(fd, &temp, 1);

            if (temp == FLAG)
                state = FLAG_RCV;

            break;

        case FLAG_RCV:
            read(fd, &frame->address, 1);

            if (frame->address == RX_ADDR || frame->address == TX_ADDR)
                state = A_RCV;
            else if (frame->address == FLAG)
                state = FLAG_RCV;
            else
                state = START;

            break;

        case A_RCV:
            read(fd, &frame->command, 1);

            if (check_command_and_address(frame, role))
                state = C_RCV;
            else if (frame->command == FLAG)
                state = FLAG_RCV;
            else
                state = START;

            break;

        case C_RCV:
            read(fd, &temp, 1);

            if (temp == (frame->address ^ frame->command))
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
                read(fd, &temp, 1);

                if (temp == FLAG)
                    state = END;
                else
                    state = START;
            }
            break;

        case DATA_RCV:
            if (frame->information == NULL)
                frame->information = bv_create();

            read(fd, &temp, 1);

            if (temp == ESC)
                state = ESC_RCV;
            else if (temp == FLAG)
                state = END_FLAG_RCV;
            else
                bv_pushb(frame->information, temp);

            break;

        case ESC_RCV:
            read(fd, &temp, 1);
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

        case END_FLAG_RCV:
            uint8_t expected_bcc2 = 0;
            uint8_t received_bcc2 = bv_popb(frame->information);

            for (size_t j = 0; j <= frame->information->length; ++j)
                expected_bcc2 ^= bv_get(frame->information, j);

            if (expected_bcc2 != received_bcc2)
                state = NACK;
            else
                state = END;
            break;

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

    bv_pushb(buf, bcc);
}

ssize_t write_frame(Frame *frame, int fd) {
    ByteVector *buf = bv_create();

    bv_pushb(buf, FLAG);
    bv_pushb(buf, frame->address);
    bv_pushb(buf, frame->command);
    bv_pushb(buf, frame->address ^ frame->command);

    if ((frame->command & 0xF) == I(0) && frame->information != NULL)
        write_info(buf, frame);

    bv_pushb(buf, FLAG);

    int bytes_written = write(fd, buf->array, buf->length);

    bv_destroy(buf);

    return bytes_written;
}

void frame_destroy(Frame* this) {

    if (this == NULL) return;

    if (this->information != NULL)
        bv_destroy(this->information);

    free(this);
}