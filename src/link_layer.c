// Link layer protocol implementation

#define _GNU_SOURCE

#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "link_layer.h"
#include "link_layer/common.h"
#include "link_layer/frame.h"
#include "link_layer/timer.h"
#include "log.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

void handshake(LLConnection *this) {
    if (this->params.role == LL_TX) {
        send_frame(this, create_frame(this, SET));
        frame_destroy(expect_frame(this, UA));

        LOG("Handshake complete\n");
    }
}

int setup_serial(LLConnection *this) {
    LOG("Setting up serial port connection...\n");

    this->fd = open(this->params.serial_port, O_RDWR | O_NOCTTY);

    if (tcgetattr(this->fd, &this->old_termios) == -1) {
        perror("llopen");
        return -1;
    }

    struct termios newtermios;
    memset(&newtermios, 0, sizeof(newtermios));

    newtermios.c_cflag = B9600 | CS8 | CLOCAL | CREAD;

    newtermios.c_iflag = IGNPAR;
    newtermios.c_oflag = 0;

    newtermios.c_lflag = 0;
    newtermios.c_cc[VTIME] = 1;
    newtermios.c_cc[VMIN] = 1;

    tcflush(this->fd, TCIOFLUSH);

    if (tcsetattr(this->fd, TCSANOW, &newtermios) == -1) {
        perror("llopen");
        return -1;
    }

    LOG("Serial port connection successfully set up!\n");

    return 1;
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
LLConnection *llopen(LLConnectionParams params) {
    LLConnection *this = malloc(sizeof(LLConnection));

    this->params = params;

    timer_setup(this);

    if (setup_serial(this) == -1)
        return NULL;

    handshake(this);

    return this;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(LLConnection *this, const unsigned char *buf, int bufSize) {
    LOG("Creating I frame!\n");

    Frame *frame = create_frame(this, I(this->tx_sequence_nr));
    frame->information = bv_create();
    bv_push(frame->information, buf, bufSize);

    LOG("Sending frame N(%d)\n", this->tx_sequence_nr);

    ssize_t bytes_written = send_frame(this, frame);

    LOG("Sent frame N(%d)\n", this->tx_sequence_nr);

    this->tx_sequence_nr = 1 - this->tx_sequence_nr;

    LOG("Expecting ACK(%d)\n", this->tx_sequence_nr);

    frame_destroy(expect_frame(this, RR(this->tx_sequence_nr)));

    LOG("Received ACK(%d)\n", this->tx_sequence_nr);

    return bytes_written;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(LLConnection *this, uint8_t *packet) {
    LOG("Waiting for I frame\n");

    Frame *frame = expect_frame(this, I(this->rx_sequence_nr));
    this->rx_sequence_nr = 1 - this->rx_sequence_nr;

    LOG("Reading I frame\n");

    memcpy(packet, frame->information->array, frame->information->length);

    LOG("Read I frame with size %lu\n", frame->information->length);

    int bytes_read = frame->information->length;

    frame_destroy(frame);

    return bytes_read;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(LLConnection *this, bool showStatistics) {
    if (this->params.role == LL_TX) {
        send_frame(this, create_frame(this, DISC));
        frame_destroy(expect_frame(this, DISC));
    } else if (!this->closed) {
        frame_destroy(expect_frame(this, DISC));
    }

    this->closed = true;

    if (tcsetattr(this->fd, TCSANOW, &this->old_termios) == -1) {
        perror("llclose");
        exit(-1); // TODO: change to return
    }

    close(this->fd);

    frame_destroy(this->last_command_frame);

    LOG("Closing serial port connection\n");

    return 1;
}
