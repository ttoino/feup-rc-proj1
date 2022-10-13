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
#include "log.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

int fd = 0;
struct termios oldtermios;

int tx_sequence_nr = 0;
int rx_sequence_nr = 0;

LinkLayerRole role;

int n_retransmissions;
int timeout;
int n_retransmissions_sent;

Frame *last_frame = NULL;

Frame *create_frame(uint8_t cmd) {
    Frame *frame = malloc(sizeof(Frame));

    frame->address =
        (IS_COMMAND(cmd) && role == LlRx) || (IS_RESPONSE(cmd) && role == LlTx)
            ? RX_ADDR
            : TX_ADDR;
    frame->command = cmd;

    return frame;
}

ssize_t send_frame(Frame *frame) {
    ssize_t bytes_written = write_frame(frame, fd);

    if (IS_COMMAND(frame->command)) {
        free(last_frame);
        last_frame = frame;
        n_retransmissions_sent = 0;
        alarm(timeout);
    } else {
        free(frame);
    }

    return bytes_written;
}

void alarm_handler(int signal) {
    if (n_retransmissions_sent == n_retransmissions) {
        ERROR("Max retries achieved, endpoints are probably disconnected, "
              "closing connection!\n");
        llclose(FALSE);
        exit(-1);
    }

    ALARM("Acknowledgement not received, retrying (c = %02x)\n", last_frame[2]);

    write_frame(last_frame, fd);
    alarm(timeout);
    n_retransmissions_sent++;
}

Frame *handle_frame(Frame *frame) {
    LOG("Received frame (c = 0x%02x)\n", frame->command);

    switch (frame->command) {
    case SET:
    case DISC:
        send_frame(create_frame(UA));
        break;
    case I(0):
    case I(1): {
        uint8_t s = frame->command >> 6;
        send_frame(create_frame(RR(1 - s)));
        break;
    }
    case UA:
    case RR(0):
    case RR(1):
        alarm(0);
        break;
    case REJ(0):
    case REJ(1):
        alarm_handler(0);
        break;
    case I_ERR | I(0):
    case I_ERR | I(1): {
        uint8_t s = frame->command >> 6;
        send_frame(create_frame(REJ(s)));
        break;
    }
    }

    LOG("Handled frame (c = 0x%02x)\n", frame->command);

    return frame;
}

Frame *expect_frame(uint8_t command) {
    Frame *frame;
    while (1) {
        frame = read_frame(fd, role);
        handle_frame(frame);

        if (frame->command == command)
            break;

        if (frame->information != NULL)
            bv_destroy(frame->information);
        free(frame);
    }
    return frame;
}

void handshake() {
    if (role == LlTx) {
        send_frame(create_frame(SET));
        expect_frame(UA);
    }

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
    role = connectionParameters.role;

    setupTimeoutHandler();

    if (setupSerialConnection(connectionParameters.serialPort, 1, 0) == -1) {
        return -1;
    }

    handshake();

    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize) {
    LOG("Creating I frame!\n");

    Frame *frame = create_frame(I(tx_sequence_nr));
    frame->information = bv_create();
    bv_push(frame->information, buf, bufSize);

    LOG("Sending frame N(%d)\n", tx_sequence_nr);

    send_frame(frame);

    LOG("Sent frame N(%d)\n", tx_sequence_nr);

    tx_sequence_nr = 1 - tx_sequence_nr;

    LOG("Expecting ACK(%d)\n", tx_sequence_nr);

    free(expect_frame(RR(tx_sequence_nr)));

    LOG("Received ACK(%d)\n", tx_sequence_nr);

    return frame->information->length;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(uint8_t *packet) {
    LOG("Waiting for I frame\n");

    Frame *frame = expect_frame(I(rx_sequence_nr));
    rx_sequence_nr = 1 - rx_sequence_nr;

    LOG("Reading I frame\n");

    memcpy(packet, frame->information->array, frame->information->length);

    LOG("Read I frame with size %d\n", frame->information->length);

    return frame->information->length;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(bool showStatistics) {
    if (tcsetattr(fd, TCSANOW, &oldtermios) == -1) {
        perror("llclose");
        exit(-1); // TODO: change to return
    }

    close(fd);

    free(last_frame);

    LOG("Closing serial port connection\n");

    return 1;
}
