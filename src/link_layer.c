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
#include "link_layer/common.h"
#include "link_layer/read_frame.h"
#include "log.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

int fd = 0;
struct termios oldtermios;

int tx_sequence_nr = 0;
int rx_sequence_nr = 0;

int n_retransmissions;
int timeout;
int n_retransmissions_sent;

unsigned char *last_frame = NULL;
size_t last_frame_size = 0;

unsigned char frame[1024];

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

    LOG("Sending supervision frame (c=0x%02x) to address 0x%02x\n", cmd, addr);

    send_frame(s_frame, S_FRAME_LEN, retry);

    free(s_frame);
}

void alarm_handler(int signal) {
    if (n_retransmissions_sent == n_retransmissions) {
        ERROR("Max retries achieved, endpoints are probably disconnected, closing connection!\n");
        llclose(FALSE);
        exit(-1);
    }

    ALARM("Acknowledgement not received, retrying (c = %02x)\n",
           last_frame[2]);

    send_frame(last_frame, last_frame_size, FALSE);
    alarm(timeout);
    n_retransmissions_sent++;
}

unsigned char handle_frame(unsigned char *frame, size_t length) {
    if (length < S_FRAME_LEN)
        return -1;

    LOG("Received frame (c = 0x%02x)\n", frame[2]);

    switch (frame[2]) {
    case SET:
    case DISC:
        send_S_frame(TX_ADDR, UA, FALSE);
        break;
    case I(0):
    case I(1): {
        unsigned char s = frame[2] >> 6;
        send_S_frame(TX_ADDR, ACK(1 - s), FALSE);
        break;
    }
    case UA:
    case ACK(0):
    case ACK(1):
        alarm(0);
        break;
    case NACK(0):
    case NACK(1):
        alarm_handler(0);
        break;
    }

    LOG("Handled frame (c = 0x%02x)\n", frame[2]);

    return frame[2];
}

size_t expect_frame(unsigned char command, unsigned char addr) {
    size_t len;
    do {
        len = read_frame(fd, frame, addr);
    } while (handle_frame(frame, len) != command);
    return len;
}

void handshake(LinkLayerRole role) {
    if (role == LlTx)
        send_S_frame(TX_ADDR, SET, TRUE);

    expect_frame(role == LlTx ? UA : SET, TX_ADDR);

    LOG("Handshake complete\n");
}

void setupTimeoutHandler() { signal(SIGALRM, alarm_handler); }

int setupSerialConnection(char *serialPort, int v_min, int v_time) {

    LOG("Setting up serial port connection...\n");

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

    LOG("Serial port connection successfully set up!\n");

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
        return -1;
    }

    handshake(connectionParameters.role);

    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
size_t create_I_frame(unsigned char addr, const unsigned char *buf,
                      size_t len) {
    frame[0] = FLAG;
    frame[1] = addr;
    frame[2] = I(tx_sequence_nr);
    frame[3] = make_BCC(frame[1], frame[2]);

    LOG("Crafted frame header for packet with length %ld\n", len);

    size_t buf_i, frame_i;
    unsigned char bcc = 0;

    for (buf_i = 0, frame_i = 4; buf_i < len; ++buf_i, ++frame_i) {

        unsigned char c = buf[buf_i];

        if (c == ESC) {
            frame[frame_i++] = ESC;
            frame[frame_i] = ESC_ESC;
        } else if (c == FLAG) {
            frame[frame_i++] = ESC;
            frame[frame_i] = ESC_FLAG;
        } else {
            frame[frame_i] = c;
        }

        bcc ^= c;
    }

    frame[frame_i++] = bcc;
    frame[frame_i++] = FLAG;

    return frame_i;
}

int llwrite(const unsigned char *buf, int bufSize) {

    LOG("Creating I frame!\n");

    size_t frame_len = create_I_frame(TX_ADDR, buf, bufSize);

    LOG("Sending frame N(%d)\n", tx_sequence_nr);

    send_frame(frame, frame_len, TRUE);

    LOG("Sent frame N(%d)\n", tx_sequence_nr);

    tx_sequence_nr = 1 - tx_sequence_nr;

    LOG("Expecting ACK(%d)\n", tx_sequence_nr);

    expect_frame(ACK(tx_sequence_nr), TX_ADDR);

    LOG("Received ACK(%d)\n", tx_sequence_nr);

    return frame_len;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
size_t read_I_frame(unsigned char *buf, size_t len) {
    size_t buf_i, frame_i;
    for (buf_i = 0, frame_i = 4; frame_i < len - 1; ++buf_i, ++frame_i) {
        unsigned char c = frame[frame_i];

        // if (c == ESC)
        //     c = frame[++frame_i] == ESC_ESC ? ESC : FLAG;

        buf[buf_i] = c;
    }
    return buf_i;
}

int llread(unsigned char *packet) {
    // TODO

    size_t len = expect_frame(I(rx_sequence_nr), TX_ADDR);
    rx_sequence_nr = 1 - rx_sequence_nr;

    LOG("Reading I frame\n");

    len = read_I_frame(packet, len);

    LOG("Read I frame with size %d\n", len);

    return len;
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

    LOG("Closing serial port connection\n");

    return 1;
}
