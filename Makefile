# -*- Makefile -*-
# Makefile to build the project
# NOTE: This file must not be changed.

# Parameters
CC = gcc

C = 9600
DEBUG_LEVEL=3
FER = 0
T_PROP = 0
PACKET_SIZE = 1024

# _DEBUG is used to include internal logging of errors and general information. Levels go from 1 to 3, highest to lowest priority respectively
# _PRINT_PACKET_DATA is used to print the packet data that is received by RX
CFLAGS = -Wall -g -D _DEBUG=$(DEBUG_LEVEL) -D C=$(C) -D FER=$(FER) -D T_PROP=$(T_PROP) -D PACKET_DATA_SIZE=$(PACKET_SIZE)

SRC = src/
INCLUDE = include/
BIN = bin/
CABLE_DIR = cable/

TX_SERIAL_PORT = /dev/ttyS10
RX_SERIAL_PORT = /dev/ttyS11

TX_FILE = neuron.jpg
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
