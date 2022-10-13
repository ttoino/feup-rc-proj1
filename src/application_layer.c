// Application layer protocol implementation

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "byte_vector.h"
#include "application_layer.h"
#include "application_layer/packet.h"
#include "link_layer.h"
#include "log.h"

int send_packet(Packet *packet) {
    ByteVector *bv = bv_create();

    bv_push(bv, &packet->type, sizeof(packet->type));

    if (packet->information != NULL)
        bv_push(bv, packet->information->array, packet->information->length);

    int result = llwrite(bv->array, bv->length);

    bv_destroy(bv);

    return result;
}

Packet* create_control_packet(enum packet_type packet_type,
                          size_t file_size, const char *file_name) {

    Packet* packet = (Packet*)malloc(sizeof(Packet));

    packet->type = packet_type;
    packet->information = NULL;

    if (packet_type == START) {
        packet->information = bv_create();

        enum control_packet_field_type field_type = FILE_SIZE;
        size_t file_size_length = sizeof(file_size);
        bv_push(packet->information, &field_type, sizeof(enum control_packet_field_type));
        bv_push(packet->information, &file_size_length, sizeof(size_t));
        bv_push(packet->information, &file_size, file_size_length);
                
        field_type = FILE_NAME;
        size_t file_name_length = strlen(file_name);
        bv_push(packet->information, &field_type, sizeof(enum control_packet_field_type));
        bv_push(packet->information, &file_name_length, sizeof(file_name_length));
        bv_push(packet->information, &file_name, file_name_length);
    }

    return packet;
}

int send_control_packet(enum packet_type packet_type, size_t file_size,
                        char *file_name) {

    Packet* packet = create_control_packet(packet_type, file_size, file_name);

    char* packet_type_name = packet_type == START ? "START" : "END";

    LOG("Sending %s control packet!\n", packet_type_name);

    int result = send_packet(packet);

    packet_destroy(packet);

    if (result == -1)
        ERROR("Could not send %s packet\n", packet_type_name);
    else
        LOG("Control packet sent\n");
    return result;
}

int send_data_packet(unsigned char *buf, size_t len) {

    // TODO: maybe make this a Packet attribute
    static unsigned char sequence_number = 0;

    Packet* packet = (Packet*)malloc(sizeof(Packet));

    packet->type = DATA;
    packet->information = bv_create();

    fill_data_packet_header(packet, len, sequence_number++);

    bv_push(packet->information, buf, len);

    int result = send_packet(packet);

    packet_destroy(packet);

    if (result == -1)
        ERROR("Could not send DATA packet with length %d\n", len);
    else
        LOG("Data packet sent\n");

    return result;
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

    if (ll.role == LlTx) {
       LOG("Connection established\n");
    }
}

/**
 * Fills in the packet header for the given packet
 */
void fill_data_packet_header(Packet* packet, int fragment_size,
                             int sequence_number) {

    if (packet == NULL) return;

    if (packet->information == NULL)
        packet->information = bv_create();

    bv_pushb(packet->information, (unsigned char)sequence_number);
    bv_pushb(packet->information, (unsigned char)((fragment_size & 0xFF00) >> 8));
    bv_pushb(packet->information, (unsigned char)(fragment_size & 0xFF));
}

int init_transmission(char* filename) {
    struct stat st;

    if (stat(filename, &st) != 0) {
        perror("Could not determine size of file to transmit");
        return -1;
    }

    if (send_control_packet(START, st.st_size, filename) == -1) {
        ERROR("Error sending START packet for file: %s\n", filename);
        return -1;
    }

    LOG("Successfully sent START packet!\n");

    return 1;
}

#define MAX_PAYLOAD_SIZE (1000 - 3 - sizeof(DATA))

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename) {

    LinkLayer ll = setupLLParams(serialPort, role, baudRate, nTries, timeout);

    connect(ll);

    unsigned char packet_data[MAX_PAYLOAD_SIZE];
    if (ll.role == LlRx) {

        unsigned char* packet_ptr = NULL;
        int fd;

        while (true) {
            int bytes_read = llread(packet_data);
            packet_ptr = packet_data;

            LOG("Processing packet\n");

            enum packet_type packet_type = *(enum packet_type *)packet_ptr;
            packet_ptr += sizeof(enum packet_type);

            // TODO: this should be refactored

            if (packet_type == END) break;
            else if (packet_type == START) {
                size_t filesize_length = *(size_t*)packet_ptr;
                packet_ptr += sizeof(size_t);

                // TODO: do something with filesize
                size_t filesize = *(size_t*)packet_ptr;
                packet_ptr += sizeof(size_t);

                size_t filename_length = *(size_t*)packet_ptr;
                packet_ptr += sizeof(size_t);

                // TODO: get filename
                packet_ptr += filename_length;

                LOG("Opening file descriptor for file: %s\n", filename);

                fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC);

                if (fd == -1) {
                    perror("opening RX fd");
                    exit(-1);
                }

            } else if (packet_type == DATA) {
                unsigned char sequence_number = *(unsigned char*)packet_ptr;
                packet_ptr += sizeof(unsigned char);

                unsigned char fragment_size_h = *(unsigned char*)packet_ptr;
                packet_ptr += sizeof(unsigned char);
                unsigned char fragment_size_l = *(unsigned char*)packet_ptr;
                packet_ptr += sizeof(unsigned char);

                uint16_t fragment_size = (fragment_size_h << 8) | fragment_size_l;

                LOG("Writing %d bytes to %s\n", fragment_size, filename);

                if (write(fd, packet_ptr, fragment_size) == -1) {
                    perror("writing to RX fd");
                    exit(-1);
                }
            }
        }
    } else {

        if (init_transmission(filename) == -1) {
            exit(-1);
        }

// TODO: Use ByteVector

        int fd = open(filename, O_RDWR | O_NOCTTY);
        if (fd == -1) {
            perror("Error opening file in transmitter!");
            exit(-1);
        }

        while (true) {
            int bytes_read = read(fd, packet_data, MAX_PAYLOAD_SIZE);

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
                send_data_packet(packet_data, bytes_read);
            }
        }
    }

    llclose(false);
}
