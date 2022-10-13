// Link layer header.
// NOTE: This file must not be changed.

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_

#include <stdbool.h>
#include <termios.h>
#include <time.h>

typedef enum _LLRole LLRole;

typedef struct _LLConnectionParams LLConnectionParams;

typedef struct _LLConnection LLConnection;

#include "link_layer/frame.h"

enum _LLRole {
    LL_TX,
    LL_RX,
};

struct _LLConnectionParams {
    char serial_port[50];
    LLRole role;
    int baud_rate;
    int n_retransmissions;
    int timeout;
};

struct _LLConnection {
    LLConnectionParams params;

    struct termios old_termios;

    int fd;
    bool closed;

    uint8_t tx_sequence_nr;
    uint8_t rx_sequence_nr;

    int n_retransmissions_sent;
    timer_t timer;
    Frame *last_command_frame;
};

// Open a connection using the "port" parameters defined in struct linkLayer.
// Return "1" on success or "-1" on error.
LLConnection *llopen(LLConnectionParams connectionParameters);

// Send data in buf with size bufSize.
// Return number of chars written, or "-1" on error.
int llwrite(LLConnection *connection, const unsigned char *buf, int bufSize);

// Receive data in packet.
// Return number of chars read, or "-1" on error.
int llread(LLConnection *connection, unsigned char *packet);

// Close previously opened connection.
// if showStatistics == TRUE, link layer should print statistics in the console
// on close. Return "1" on success or "-1" on error.
int llclose(LLConnection *connection, bool showStatistics);

#endif // _LINK_LAYER_H_
