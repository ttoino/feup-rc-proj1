// Application layer protocol implementation

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "application_layer.h"
#include "link_layer.h"
#include "log.h"

unsigned char n = 0;

int create_control_packet(unsigned char **packet, enum packet_type packet_type,
                          size_t file_size, const char *file_name) {

    int size = 0;
    unsigned char* control_packet = NULL;

    if (packet_type == END) {
        control_packet = malloc(1 * sizeof(unsigned char));
        *control_packet = END;
        size = 1;
    } else if (packet_type == START) {

        // TODO: lengths maybe should be normalized

        size = sizeof(START) +
                //      type                length                  value
                sizeof(FILE_SIZE) + sizeof(sizeof(file_size)) + sizeof(file_size) +
                sizeof(FILE_NAME) + sizeof(strlen(file_name)) + strlen(file_name);

        control_packet = malloc(size * sizeof(unsigned char));

        size_t offset = 0;

        memcpy(control_packet + offset, &packet_type, sizeof(enum packet_type));
        offset += sizeof(enum packet_type);

        enum control_packet_field_type field_type = FILE_SIZE;
        memcpy(control_packet + offset, &field_type, sizeof(enum control_packet_field_type));
        offset += sizeof(enum control_packet_field_type);

        unsigned long file_size_length = sizeof(file_size);
        memcpy(control_packet + offset, &file_size_length, sizeof(file_size_length));
        offset += sizeof(file_size_length);

        memcpy(control_packet + offset, &file_size, file_size_length);
        offset += file_size_length;

        field_type = FILE_NAME;
        memcpy(control_packet + offset, &field_type, sizeof(enum control_packet_field_type));
        offset += sizeof(enum control_packet_field_type);

        unsigned long file_name_length = strlen(file_name);
        memcpy(control_packet + offset, &file_name_length, sizeof(file_name_length));
        offset += sizeof(file_size_length);

        memcpy(control_packet + offset, &file_name, file_name_length);
        offset += file_name_length;

    } else
        printf("%d is not a valid control packet indicator\n", packet_type);

    *packet = control_packet;
    return size;
}

int send_control_packet(enum packet_type packet_type, size_t file_size,
                        char *file_name) {

    unsigned char *control_packet = NULL;
    int packet_size = create_control_packet(&control_packet, packet_type,
                                            file_size, file_name);

    LOG("Sending control packet!\n");

    if (llwrite(control_packet, packet_size) == -1) {
        ERROR("Could not send START packet\n");

        return -1;
    }
    free(control_packet);

    LOG("Control packet sent\n");
    return 1;
}

LinkLayer setupLLParams(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout) {
    LinkLayer ll = {
        .baudRate = baudRate, .nRetransmissions = nTries, .timeout = timeout};

    strncpy(ll.serialPort, serialPort, sizeof(ll.serialPort) - 1);

    LinkLayerRole r = LlTx;
    if (strcmp(role, "rx") == 0)
        r = LlRx;

    ll.role = r;

    return ll;
}

void connect(LinkLayer ll) {

    LOG("Connecting to %s\n", ll.serialPort);

    if (llopen(ll) == -1) {
        ERROR("Serial connection on port %s not available, aborting\n",
               ll.serialPort);
        exit(-1);
    }

    LOG("Connection established\n");
}

/**
 * Fills in the packet header for the given packet
 */
void fill_data_packet_header(unsigned char *packet, int fragment_size,
                             int sequence_number) {

    packet[0] = DATA;
    packet[1] = (unsigned char)sequence_number;
    packet[2] = (unsigned char)((fragment_size & 0xFF00) >> 8);
    packet[3] = (unsigned char)(fragment_size & 0xFF);
}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename) {

    LinkLayer ll = setupLLParams(serialPort, role, baudRate, nTries, timeout);

    connect(ll);

    unsigned char s[1024];
    if (ll.role == LlRx) {
        while (TRUE) {
            llread(s);
            printf("\"%s\"\n", s);
        }
    } else {

        struct stat st;

        if (stat(filename, &st) != 0) {
            perror("Could not determine size of file to transmit");
            exit(-1);
        }

        if (send_control_packet(START, st.st_size, filename) == -1) {
            ERROR("Error sending START packet for file: %s\n", filename);
            exit(-1);
        }

        LOG("Successfully sent START packet!\n");

        unsigned char packet[MAX_PAYLOAD_SIZE];
        while (TRUE) {
            int fd = open(filename, O_RDWR | O_NOCTTY);

            if (fd == -1) {
                perror("Error opening file in transmitter!");
                exit(-1);
            }

            int bytes_read = read(fd, (packet + 4), MAX_PAYLOAD_SIZE - 4);

            if (bytes_read == -1) {
                perror("Error reading file fragment, aborting");
                exit(-1);
            } else if (bytes_read == 0) {
                // reached end of file, send END packet

                if (send_control_packet(END, 0, "") == -1) {
                    ERROR("Error sending END control packet\n");
                    exit(-1);
                }

                break;
            } else {
                fill_data_packet_header(packet, bytes_read, n++);

                // send DATA packet
                if (llwrite(packet, bytes_read + 4) == -1) {
                    ERROR("Error sending data packet %d\n", n - 1);
                }
            }
        }
    }

    llclose(FALSE);
}
