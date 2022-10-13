#include "link_layer/frame.h"

#include <unistd.h>

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

    return write(fd, buf->array, buf->length);
}
