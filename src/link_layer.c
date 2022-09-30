// Link layer protocol implementation

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "link_layer.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

int fd = 0;
struct termios oldtermios;

unsigned char make_BCC(unsigned char addr, unsigned char cmd) {
    return (unsigned char)(addr ^ cmd);
}

void read_S_frame(unsigned char addr, unsigned char cmd) {
#define READ_S_FRAME_BYTE(EXPECTED)                                            \
    read(fd, &byte, 1);                                                        \
    if (byte != EXPECTED) {                                                    \
        if (byte == FLAG)                                                      \
            goto flag_rcv;                                                     \
        else                                                                   \
            goto start;                                                        \
    }

    unsigned char byte;

start:
    READ_S_FRAME_BYTE(FLAG);

flag_rcv:
    READ_S_FRAME_BYTE(addr);
    READ_S_FRAME_BYTE(cmd);
    READ_S_FRAME_BYTE(make_BCC(addr, cmd));
    READ_S_FRAME_BYTE(FLAG);
}

unsigned char *create_S_frame(unsigned char addr, unsigned char cmd) {

    unsigned char *frame = calloc(S_FRAME_LEN, sizeof(unsigned char));

    frame[0] = FLAG;
    frame[1] = addr;
    frame[2] = cmd;
    frame[3] = make_BCC(addr, cmd);
    frame[4] = FLAG;

    return frame;
}

size_t send_frame(unsigned char *frame, size_t frame_len) {
    return write(fd, frame, frame_len);
}

void send_S_frame(unsigned char addr, unsigned char cmd) {
    unsigned char *set_frame = create_S_frame(addr, cmd);

    send_frame(set_frame, S_FRAME_LEN);

    free(set_frame);
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters) {

    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);

    if (tcgetattr(fd, &oldtermios) == -1) {
        perror("llopen");
        exit(-1);
    }

    struct termios newtermios;
    memset(&newtermios, 0, sizeof(newtermios));

    newtermios.c_cflag = B9600 | CS8 | CLOCAL | CREAD;

    newtermios.c_iflag = IGNPAR;
    newtermios.c_oflag = 0;

    newtermios.c_lflag = 0;
    newtermios.c_cc[VTIME] = 0;
    newtermios.c_cc[VMIN] = 1;

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtermios) == -1) {
        perror("llopen");
        exit(-1);
    }

    if (connectionParameters.role == LlTx) {
        send_S_frame(TX_ADDR, SET);
        read_S_frame(TX_ADDR, UA);
    } else {
        read_S_frame(TX_ADDR, SET);
        send_S_frame(TX_ADDR, UA);
    }

    return 0;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize) {
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet) {
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics) {
    if (tcsetattr(fd, TCSANOW, &oldtermios) == -1) {
        perror("llclose");
        exit(-1); // TODO: change to return
    }

    close(fd);

    return 1;
}
