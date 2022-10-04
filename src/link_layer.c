// Link layer protocol implementation

#define _GNU_SOURCE

#include <fcntl.h>
#include <signal.h>
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

int n_retransmissions;
int timeout;
int n_retransmissions_sent;

unsigned char *last_frame = NULL;
size_t last_frame_size = 0;

unsigned char make_BCC(unsigned char addr, unsigned char cmd) {
    return (unsigned char)(addr ^ cmd);
}

unsigned char *read_frame(unsigned char addr, unsigned char cmd) {
    // AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA this will change
#define READ_FRAME_HEADER(EXPECTED)                                            \
    read(fd, frame + i, 1);                                                    \
    if (frame[i++] != (EXPECTED)) {                                            \
        if (frame[i - 1] == FLAG)                                              \
            goto flag_rcv;                                                     \
        else                                                                   \
            goto start;                                                        \
    }

    unsigned char *frame = calloc(S_FRAME_LEN, sizeof(unsigned char));
    size_t i = 0;

start:
    i = 0;
    READ_FRAME_HEADER(FLAG);

flag_rcv:
    i = 1;
    READ_FRAME_HEADER(addr);
    READ_FRAME_HEADER(cmd);
    READ_FRAME_HEADER(make_BCC(addr, cmd));

    if (frame[2] & 0xF == I(0))
        while (TRUE) {
            frame = reallocarray(frame, i + 1, sizeof(unsigned char));
            read(fd, frame + i, 1);

            if (frame[i] == ESC) {
                unsigned char temp;
                read(fd, &temp, 1);
                if (temp == ESC_FLAG) {
                    frame[i] = FLAG;
                } else if (temp == ESC_ESC) {
                    frame[i] = ESC;
                } else {
                    frame[2] |= I_ERR;
                    break;
                }
            } else if (frame[i] == FLAG) {
                unsigned char bcc2 = 0;

                for (size_t j = 4; j <= i - 2; ++j)
                    bcc2 ^= frame[j];

                if (bcc2 != frame[i - 1])
                    frame[2] |= I_ERR;

                break;
            }
        }
    else
        READ_FRAME_HEADER(FLAG);

    return frame;
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

ssize_t send_frame(unsigned char *frame, size_t frame_len, int retry) {
    ssize_t bytes_written = write(fd, frame, frame_len);

    if (retry) {
        last_frame = reallocarray(last_frame, frame_len, sizeof(unsigned char));
        memcpy(last_frame, frame, frame_len * sizeof(unsigned char));
        last_frame_size = frame_len;
        n_retransmissions_sent = 0;
        alarm(timeout);
    }

    return bytes_written;
}

void send_S_frame(unsigned char addr, unsigned char cmd, int retry) {
    unsigned char *s_frame = create_S_frame(addr, cmd);

    send_frame(s_frame, S_FRAME_LEN, retry);

    free(s_frame);
}

void alarm_handler(int signal) {
    if (n_retransmissions_sent == n_retransmissions)
        exit(-1);

    puts("Acknowledgement not received, retrying");

    send_frame(last_frame, last_frame_size, FALSE);
    alarm(timeout);
    n_retransmissions_sent++;
}

void handshake(LinkLayerRole role) {
    if (role == LlTx) {
        send_S_frame(TX_ADDR, SET, TRUE);
        free(read_frame(TX_ADDR, UA));
        alarm(0);
    } else {
        free(read_frame(TX_ADDR, SET));
        send_S_frame(TX_ADDR, UA, FALSE);
    }
}

void setupTimeoutHandler() { signal(SIGALRM, alarm_handler); }

int setupSerialConnection(char *serialPort, int v_min, int v_time) {

    fd = open(serialPort, O_RDWR | O_NOCTTY);

    if (tcgetattr(fd, &oldtermios) == -1) {
        perror("llopen");
        return -1;
    }

    struct termios newtermios;
    memset(&newtermios, 0, sizeof(newtermios));

    newtermios.c_cflag = B9600 | CS8 | CLOCAL | CREAD;

    newtermios.c_iflag = IGNPAR;
    newtermios.c_oflag = 0;

    newtermios.c_lflag = 0;
    newtermios.c_cc[VTIME] = v_time;
    newtermios.c_cc[VMIN] = v_min;

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtermios) == -1) {
        perror("llopen");
        return -1;
    }

    return 1;
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters) {
    n_retransmissions = connectionParameters.nRetransmissions;
    n_retransmissions_sent = 0;
    timeout = connectionParameters.timeout;

    setupTimeoutHandler();

    if (setupSerialConnection(connectionParameters.serialPort, 1, 0) == -1) {
        exit(-1);
    }

    handshake(connectionParameters.role);

    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize) {
    // TODO

    return -1;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet) {
    // TODO

    return -1;
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

    free(last_frame);
    last_frame_size = 0;

    return 1;
}
