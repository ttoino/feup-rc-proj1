// Link layer protocol implementation

#define _GNU_SOURCE

#include <errno.h>
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
#include "link_layer/frame.h"
#include "link_layer/timer.h"
#include "log.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

int handshake(LLConnection *this) {
    if (this->params.role == LL_TX) {
        if (send_frame(this, create_frame(this, SET)) == -1)
            return -1;

        Frame *f = expect_frame(this, UA);
        if (f == NULL)
            return -1;
        frame_destroy(f);

        LOG("Handshake complete\n");
    }

    return 0;
}

int setup_serial(LLConnection *this) {
    LOG("Setting up serial port connection...\n");

    this->fd = open(this->params.serial_port, O_RDWR | O_NOCTTY);

    if (this->fd == -1) {
        ERROR("llopen: %s\n", strerror(errno));
        return -1;
    }

    if (tcgetattr(this->fd, &this->old_termios) == -1) {
        ERROR("llopen: %s\n", strerror(errno));
        return -1;
    }

    struct termios newtermios;
    memset(&newtermios, 0, sizeof(newtermios));

    newtermios.c_cflag = B230400 | CS8 | CLOCAL | CREAD;

    newtermios.c_iflag = IGNPAR;
    newtermios.c_oflag = 0;

    newtermios.c_lflag = 0;
    newtermios.c_cc[VTIME] = 1;
    newtermios.c_cc[VMIN] = 1;

    if (tcflush(this->fd, TCIOFLUSH) == -1) {
        ERROR("llopen: %s\n", strerror(errno));
        return -1;
    }

    if (tcsetattr(this->fd, TCSANOW, &newtermios) == -1) {
        ERROR("llopen: %s\n", strerror(errno));
        return -1;
    }

    LOG("Serial port connection successfully set up!\n");

    return 1;
}

void connection_destroy(LLConnection *this) {
    frame_destroy(this->last_command_frame);
    tcsetattr(this->fd, TCSANOW, &this->old_termios);
    close(this->fd);
    timer_destroy(this);
    free(this);
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
LLConnection *llopen(LLConnectionParams params) {
    LLConnection *this = malloc(sizeof(LLConnection));
    this->last_command_frame = NULL;

    this->params = params;

    if (setup_serial(this) == -1) {
        connection_destroy(this);
        return NULL;
    }

    if (timer_setup(this) == -1) {
        connection_destroy(this);
        return NULL;
    }

    if (handshake(this) == -1) {
        connection_destroy(this);
        return NULL;
    }

    return this;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
ssize_t llwrite(LLConnection *this, const uint8_t *buf, size_t bufSize) {
    if (this->closed)
        return -1;

    LOG("Creating I frame!\n");

    Frame *frame = create_frame(this, I(this->tx_sequence_nr));

    if (frame == NULL)
        return -1;

    frame->information = bv_create();
    bv_push(frame->information, buf, bufSize);

    LOG("Sending frame I(%d)\n", this->tx_sequence_nr);

    ssize_t bytes_written = send_frame(this, frame);

    if (bytes_written == -1)
        return -1;

    this->tx_sequence_nr = 1 - this->tx_sequence_nr;

    LOG("Expecting RR(%d)\n", this->tx_sequence_nr);

    Frame *f = expect_frame(this, RR(this->tx_sequence_nr));
    if (f == NULL)
        return -1;
    frame_destroy(f);

    return bytes_written;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
ssize_t llread(LLConnection *this, uint8_t *packet) {
    if (this->closed)
        return -1;

    LOG("Waiting for I frame\n");

    Frame *frame = expect_frame(this, I(this->rx_sequence_nr));

    if (frame == NULL)
        return -1;

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
    if (!this->closed) {
        if (this->params.role == LL_TX) {
            send_frame(this, create_frame(this, DISC));
            frame_destroy(expect_frame(this, DISC));
        } else {
            frame_destroy(expect_frame(this, DISC));
        }
    }

    connection_destroy(this);

    LOG("Closing serial port connection\n");

    return 1;
}
