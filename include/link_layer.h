// Link layer header.
// NOTE: This file must not be changed.

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_

typedef enum {
    LlTx,
    LlRx,
} LinkLayerRole;

typedef struct {
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;

// SIZE of maximum acceptable payload.
// Maximum number of bytes that application layer should send to link layer
#define MAX_PAYLOAD_SIZE 1000

// MISC
#define TRUE 1
#define FALSE 0

#define BIT_B(b, n) ((b)<<(n))
#define BIT(n) BIT_B(1, n)

#define S_FRAME_LEN 5

#define FLAG (unsigned char)0x7e

#define RX_ADDR (unsigned char)0x03
#define TX_ADDR (unsigned char)0x07

#define UA (unsigned char)0x07
#define SET (unsigned char)0x03
#define DISC (unsigned char)0x0B
#define I(s) (unsigned char)BIT_B(s, 6)
#define ACK(r) (unsigned char)(BIT_B(r, 7) | 0b101)
#define NACK(r) (unsigned char)(BIT_B(r, 7) | 0b001)

// Open a connection using the "port" parameters defined in struct linkLayer.
// Return "1" on success or "-1" on error.
int llopen(LinkLayer connectionParameters);

// Send data in buf with size bufSize.
// Return number of chars written, or "-1" on error.
int llwrite(const unsigned char *buf, int bufSize);

// Receive data in packet.
// Return number of chars read, or "-1" on error.
int llread(unsigned char *packet);

// Close previously opened connection.
// if showStatistics == TRUE, link layer should print statistics in the console
// on close. Return "1" on success or "-1" on error.
int llclose(int showStatistics);

#endif // _LINK_LAYER_H_
