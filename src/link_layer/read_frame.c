#include "link_layer/read_frame.h"
#include "link_layer.h"
#include "link_layer/common.h"

#include <unistd.h>

typedef enum {
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_RCV,
    DATA_RCV,
    ESC_RCV,
    END_FLAG_RCV,
    END,
    NACK,
} ReadFrameState;

ReadFrameState read_header_byte(int fd, unsigned char *dest,
                                unsigned char expected,
                                ReadFrameState next_state) {
    read(fd, dest, 1);
    if (*dest == expected)
        return next_state;
    if (*dest == FLAG)
        return FLAG_RCV;
    else
        return START;
}

size_t read_frame(int fd, unsigned char *frame, unsigned char addr) {
    ReadFrameState state = START;
    size_t i = 3;

    while (TRUE) {
        switch (state) {
        case START:
            state = read_header_byte(fd, frame, FLAG, FLAG_RCV);
            break;

        case FLAG_RCV:
            state = read_header_byte(fd, frame + 1, addr, A_RCV);
            break;

        case A_RCV:
            read(fd, frame + 2, 1);

            if (frame[2] == FLAG)
                state = FLAG_RCV;
            else
                state = C_RCV;

            break;

        case C_RCV:
            state = read_header_byte(fd, frame + 3,
                                     make_BCC(frame[1], frame[2]), BCC_RCV);
            break;

        case BCC_RCV:
            if ((frame[2] & 0xF) == I(0))
                state = DATA_RCV;
            else
                state = read_header_byte(fd, frame + (++i), FLAG, END);
            break;

        case DATA_RCV:
            read(fd, frame + (++i), 1);

            if (frame[i] == ESC)
                state = ESC_RCV;
            else if (frame[i] == FLAG)
                state = END_FLAG_RCV;

            break;

        case ESC_RCV:
            unsigned char temp;
            read(fd, &temp, 1);
            state = DATA_RCV;

            if (temp == ESC_ESC)
                frame[i] = ESC;
            else if (temp == ESC_FLAG)
                frame[i] = FLAG;
            else
                state = NACK;

            break;

        case NACK:
            frame[2] |= I_ERR;
            state = END;
            break;

        case END_FLAG_RCV:
            unsigned char bcc2 = 0;

            for (size_t j = 4; j <= i - 2; ++j)
                bcc2 ^= frame[j];

            if (bcc2 != frame[i - 1])
                state = NACK;
            else
                state = END;
            break;
        default:
            return i + 1;
        }
    }
}
