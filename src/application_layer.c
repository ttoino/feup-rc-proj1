// Application layer protocol implementation

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "byte_vector.h"
#include "link_layer.h"
#include "log.h"

#include "application_layer.h"
#include "application_layer/packet.h"

LLConnectionParams setupLLParams(const char *serial_port, const char *role,
                                 int baud_rate, int nTries, int timeout) {
    LLConnectionParams ll = {.baud_rate = baud_rate,
                             .n_retransmissions = nTries,
                             .timeout = timeout,
                             .role = strcmp(role, "rx") == 0 ? LL_RX : LL_TX};

    strncpy(ll.serial_port, serial_port, sizeof(ll.serial_port) - 1);

    return ll;
}

LLConnection *connect(LLConnectionParams ll) {
    LOG("Connecting to %s\n", ll.serial_port);

    LLConnection *connection = llopen(ll);

    if (connection == NULL) {
        ERROR("Serial connection on port %s not available, aborting\n",
              ll.serial_port);
        exit(-1);
    }

    if (ll.role == LL_TX) {
        LOG("Connection established\n");
    }

    return connection;
}

int init_transmission(LLConnection *connection, char *filename) {
    struct stat st;

    if (stat(filename, &st) != 0) {
        perror("Could not determine size of file to transmit");
        return -1;
    }

    if (send_packet(connection, create_start_packet(st.st_size, filename)) ==
        -1) {
        ERROR("Error sending START packet for file: %s\n", filename);
        return -1;
    }

    LOG("Successfully sent START packet!\n");

    return 1;
}

ssize_t receiver(LLConnection *connection) {
    uint8_t *packet_ptr = NULL;
    int fd = -1;
    char file_name[256 + 9]; // Give space for "_received"
    size_t file_size = 0;
    uint8_t packet[PACKET_SIZE];

    while (true) {
        ssize_t bytes_read = llread(connection, packet);

        if (bytes_read == -1) {
            ERROR("Invalid read!\n");
            break;
        }

        INFO("Received packet:");
        for (size_t i = 0; i < bytes_read; ++i)
            printf(" %02x", packet[i]);
        printf("\n");

        packet_ptr = packet;

        LOG("Processing packet\n");

        uint8_t packet_type = *packet_ptr++;

        if (packet_type == END_PACKET)
            break;
        else if (packet_type == START_PACKET) {
            for (uint8_t *packet_end = packet + bytes_read;
                 packet_ptr < packet_end;) {
                uint8_t type = *packet_ptr++;
                uint8_t size = *packet_ptr++;

                switch (type) {
                case FILE_SIZE_FIELD:
                    for (uint8_t i = 0; i < size; ++i)
                        file_size += *packet_ptr++ << (8 * i);
                    break;
                case FILE_NAME_FIELD:
                    memcpy(&file_name, packet_ptr, size);
                    packet_ptr += size;
                    break;
                }
            }

            INFO("Transferring file %s with size %lu\n", file_name, file_size);

            sprintf(file_name + strlen(file_name), "_received");

            LOG("Opening file descriptor for file: %s\n", file_name);

            fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);

            if (fd == -1) {
                perror("opening RX fd");
                exit(-1);
            }

        } else if (packet_type == DATA_PACKET) {
            uint8_t sequence_number = *packet_ptr++;
            uint8_t fragment_size_h = *packet_ptr++;
            uint8_t fragment_size_l = *packet_ptr++;

            uint16_t fragment_size = (fragment_size_h << 8) | fragment_size_l;

            LOG("Writing %d bytes to %s\n", fragment_size, file_name);

            ssize_t bytes_written = write(fd, packet_ptr, fragment_size);
            static size_t total_bytes_written = 0;

            if (bytes_written == -1) {
                perror("writing to RX fd");
                exit(-1);
            } else if (bytes_written != 0) {
                total_bytes_written += bytes_written;

                INFO("Written %lf%% of the file\n",
                     (double)(total_bytes_written * 100.0 / file_size));
            }
        }
    }

    if (fd != -1)
        close(fd);
}

ssize_t transmitter(LLConnection *connection, const char *filename) {
    int fd = open(filename, O_RDWR | O_NOCTTY);

    if (fd == -1) {
        ERROR("Error opening file!");
        return -1;
    }

    if (init_transmission(connection, filename) == -1)
        return -1;

    uint8_t packet_data[PACKET_DATA_SIZE];

    while (true) {
        int bytes_read = read(fd, packet_data, PACKET_DATA_SIZE);

        if (bytes_read == -1) {
            ERROR("Error reading file fragment, aborting");
            break;

        } else if (bytes_read == 0) {
            // reached end of file, send END packet

            if (send_packet(connection, create_end_packet()) == -1) {
                ERROR("Error sending END control packet\n");
            }

            break;
        } else {
            if (send_packet(connection, create_data_packet(packet_data,
                                                           bytes_read)) == -1) {
                ERROR("Error sending DATA packet\n");
                break;
            };
        }
    }

    close(fd);
}

void applicationLayer(const char *serial_port, const char *role, int baud_rate,
                      int n_tries, int timeout, const char *filename) {
    LLConnectionParams ll =
        setupLLParams(serial_port, role, baud_rate, n_tries, timeout);

    LLConnection *connection = connect(ll);

    if (connection == NULL) {
        ERROR("Error establishing connection.");
    }

    if (ll.role == LL_RX) {
        receiver(connection);
    } else {
        transmitter(connection, filename);
    }

    llclose(connection, false);
}
