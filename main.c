// Main file of the serial port project.
// NOTE: This file must not be changed.

#define _POSIX_SOURCE 1
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "application_layer.h"

// Arguments:
//   $1: /dev/ttySxx
//   $2: tx | rx
//   $3: filename
int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: %s /dev/ttySxx tx|rx filename\n", argv[0]);
        exit(1);
    }

    const char *serial_port = argv[1];
    const char *role = argv[2];
    const char *filename = argv[3];

    srand(time(NULL));

    application_layer(serial_port, role, filename);

    return 0;
}
