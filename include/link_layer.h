#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_

#include <stdbool.h>
#include <termios.h>
#include <time.h>

#undef LOG_NAME
#define LOG_NAME "LINK LAYER"

#ifndef FER
#define FER 0
#endif
#ifndef T_PROP
#define T_PROP 0
#endif
#ifndef C
#define C 9600
#endif
#define JOIN(a, b) a##b
#define BAUDRATE(c) JOIN(B, c)
#ifndef N_TRIES
#define N_TRIES 3
#endif
#ifndef TIMEOUT
#define TIMEOUT 4
#endif

/**
 * @brief An enum representing the role of a connection.
 */
typedef enum _LLRole LLRole;

/**
 * @brief A struct representing a connection and its state.
 */
typedef struct _LLConnection LLConnection;

#include "link_layer/frame.h"

/**
 * @brief An enum representing the role of a connection.
 */
enum _LLRole {
    /**
     * @brief The transmitter role.
     */
    LL_TX,
    /**
     * @brief The receiver role.
     */
    LL_RX,
};

/**
 * @brief A struct representing a connection and its state.
 */
struct _LLConnection {
    /**
     * @brief The role of this connection.
     */
    LLRole role;

    /**
     * @brief The serial port config present before the connection was setup.
     */
    struct termios old_termios;

    /**
     * @brief The file descriptor of the serial port used in this connection.
     *
     * @note Is invalid if the connection has been closed.
     */
    int fd;
    /**
     * @brief Whether this connection has been closed.
     */
    bool closed;

    /**
     * @brief The next sequence number to be used when sending an I frame.
     */
    uint8_t tx_sequence_nr;
    /**
     * @brief The next sequence number to be expected when receiving an I frame.
     */
    uint8_t rx_sequence_nr;

    /**
     * @brief The number of retransmissions already sent.
     */
    int n_retransmissions_sent;
    /**
     * @brief The POSIX timer used to resend frames on an interval.
     */
    timer_t timer;
    /**
     * @brief The last command frame sent by this connection.
     */
    Frame *last_command_frame;
};

/**
 * @brief Open a connection using the specified parameters.
 *
 * @param connectionParameters The parameters to use.
 *
 * @return The newly opened connection.
 * @return NULL on error.
 */
LLConnection *llopen(const char *serial_port, LLRole role);

/**
 * @brief Send data through a connection.
 *
 * @param connection The connection to send data through.
 * @param buf The data to send.
 * @param buf_len The length of the data.
 *
 * @return The number of bytes written.
 * @return Negative on error.
 */
ssize_t llwrite(LLConnection *connection, const uint8_t *buf, size_t buf_len);

/**
 * @brief Receive data from a connection.
 *
 * @param connection The connection to receive data from.
 * @param buf Where to store the data.
 *
 * @return The number of bytes read.
 * @return Negative on error.
 */
ssize_t llread(LLConnection *connection, uint8_t *buf);

/**
 * @brief Closes a previously opened connection.
 *
 * @param connection The connection to be closed.
 * @param show_stats Whether to print connection statistics.
 *
 * @return 1 on success.
 * @return Negative on error
 */
int llclose(LLConnection *connection);

#endif // _LINK_LAYER_H_
