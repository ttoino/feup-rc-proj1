// Application layer protocol implementation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "application_layer.h"
#include "link_layer.h"

unsigned char n = 0;

int create_control_packet(unsigned char* packet, enum packet_type packet_type, size_t file_size, const char* file_name) {

    int size = 0;

    if (packet_type == END) {
	packet = malloc(1*sizeof(unsigned char));
	*packet = END;
	size = 1;
    } else if (packet_type == START) {
    	size = sizeof(enum packet_type) + 2*sizeof(enum control_packet_field_type) + 2 + sizeof(file_size) + sizeof(strlen(file_name));

	packet = calloc(size, sizeof(unsigned char));

	packet[0] = START;
	packet[1] = FILE_SIZE;
	packet[2] = sizeof(file_size);
	memcpy(packet + 3, &file_size , packet[2]);
	packet[4 + packet[2]] = FILE_NAME;
	packet[5 + packet[2]] = strlen(file_name);
	strncpy(packet + 6, file_name, packet[5 + packet[2]]);

    } else
	printf("%d is not a valid control packet indicator\n", packet_type);

    return size;
}

int send_control_packet(enum packet_type packet_type, size_t file_size, char* file_name) {

    unsigned char* control_packet = NULL;
    int packet_size = create_control_packet(control_packet, packet_type, file_size, file_name);

    if (llwrite(control_packet, packet_size) == -1) {
	puts("Could not send START packet");

	return -1;
    }
    free(control_packet);
    return 1;
}

/**
 * Fills in the packet header for the given packet
 */
void fill_data_packet_header(unsigned char* packet, int fragment_size, int sequence_number) {

    packet[0] = DATA;
    packet[1] = (unsigned char) sequence_number;
    packet[2] = (unsigned char) ((fragment_size & 0xFF00) >> 8);
    packet[3] = (unsigned char) (fragment_size & 0xFF);
}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename) {
    LinkLayer ll = {
        .baudRate = baudRate, .nRetransmissions = nTries, .timeout = timeout};

    strncpy(ll.serialPort, serialPort, sizeof(ll.serialPort) - 1);

    LinkLayerRole r = LlTx;
    if (strcmp(role, "rx") == 0)
        r = LlRx;

    ll.role = r;

    if (llopen(ll) == -1) {
	printf("Serial connection on port %s not available, aborting\n", serialPort);
	exit(-1);
    }

    if (ll.role == LlRx) {
        unsigned char *s = NULL;
        while (llread(s) != -1)
            printf("\"%s\"\n", s);
        free(s);
    } else {

	int fd = open(filename, O_RDWR | O_NOCTTY);

	if (fd == -1) {
	    perror("Error opening file in transmitter!");
	}

	struct stat st;

	if (stat(filename, &st) != 0) {
	    perror("Could not determine size of file to transmit");
	    exit(-1);
	}

	if (send_control_packet(START, st.st_size, filename) == -1) {
	    printf("Error sending START packet for file: %s\n", filename);
	    exit(-1);
	}

	unsigned char packet[MAX_PAYLOAD_SIZE];
	while (TRUE) {
	    int bytes_read = read(fd, (packet+4), MAX_PAYLOAD_SIZE-4);

	    if (bytes_read == -1) {
		perror("Error reading file fragment, aborting");
		exit(-1);
	   } else if (bytes_read == 0) {
		// reached end of file, send END packet

		if (send_control_packet(END, 0, "") == -1) {
		    printf("Error sending END control packet\n");
		    exit(-1);
		}

		break;
	    } else {
		fill_data_packet_header(packet, bytes_read, n++);

		// send DATA packet
		if (llwrite(packet, bytes_read+4) == -1) {
		    printf("Error sending data packet %d\n", n-1);
		}
	    }
	}
    }

    llclose(FALSE);
}
