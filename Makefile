# -*- Makefile -*-

# Parameters
CC = gcc

DEBUG_LEVEL=3
# Connection capacity, aka baudrate
C = 57600
# Frame error ratio
FER = 0
# Propagation time
T_PROP = 0
# Data packet data size
PACKET_SIZE = 4096

# _DEBUG is used to include internal logging of errors and general information.
# Levels go from 1 to 3, highest to lowest priority respectively
# _PRINT_PACKET_DATA is used to print the packet data that is received by RX
CFLAGS = -Wall -g -D _DEBUG=$(DEBUG_LEVEL) -D C=$(C) -D FER=$(FER) -D T_PROP=$(T_PROP) -D PACKET_DATA_SIZE=$(PACKET_SIZE)

SRC = src/
INCLUDE = include/
BIN = bin/
CABLE_DIR = cable/

# The serial port for the transmitter
TX_SERIAL_PORT = /dev/ttyS10
# The serial port for the receiver
RX_SERIAL_PORT = /dev/ttyS11

# The file to be transmitted
TX_FILE = neuron.jpg
# Does nothing
RX_FILE = neuron-received.jpg

# Targets
.PHONY: all
all: $(BIN)/main $(BIN)/cable

$(BIN)/main: main.c $(SRC)/**/*.c $(SRC)/*.c
	$(CC) $(CFLAGS) -o $@ $^ -I$(INCLUDE) -lrt

$(BIN)/cable: $(CABLE_DIR)/cable.c
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: run_tx
run_tx: $(BIN)/main
	./$(BIN)/main $(TX_SERIAL_PORT) tx $(TX_FILE)

.PHONY: run_rx
run_rx: $(BIN)/main
	./$(BIN)/main $(RX_SERIAL_PORT) rx $(RX_FILE)

.PHONY: run_cable
run_cable: $(BIN)/cable
	./$(BIN)/cable

docs: $(BIN)/main
	doxygen Doxyfile

.PHONY: check_files
check_files:
	diff -s $(TX_FILE) $(RX_FILE) || exit 0

.PHONY: clean
clean:
	rm -f $(BIN)/main
	rm -f $(BIN)/cable
	rm -f $(RX_FILE)
