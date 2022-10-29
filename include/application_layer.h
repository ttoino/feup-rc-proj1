// Application layer protocol header.
// NOTE: This file must not be changed.

#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

/**
 * @brief Performs the application layer routine.
 * 
 * @param serial_port the serial port to connect to
 * @param role the role of this instance of the application
 * @param filename the name of the file to transmit if this instance is a transmitter
 */
void application_layer(const char *serial_port, const char *role,
                       const char *filename);

#endif // _APPLICATION_LAYER_H_
