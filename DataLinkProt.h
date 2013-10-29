#ifndef DATA_LINK_PROT_H_
#define DATA_LINK_PROT_H_

#include "macros.h"

typedef struct linkLayer {
	char port[20];
	unsigned int baudRate;
	unsigned char sequenceNumber;
	unsigned int timeout;
	unsigned int numTransmissions;
	unsigned int isOpen;
	int status;
	unsigned int headerErrorRate;
	unsigned int frameErrorRate;
} linkLayer_t;

/**
 * @param port 
 * @param status flag  identifying the current mode (TRANSMITTER | RECEIVER)
 * @return data link identifier, or -1 if error
 */
int llopen(unsigned int port, unsigned int status);

/**
 * @param fd data link identifier
 * @param buffer chars array to be filled with data (already allocated)
 * @return array length, or -1 if error
 */
int llread(int fd, unsigned char * buffer);

/**
 * @param fd data link identifier
 * @param buffer chars array to be sent
 * @param length array buffer
 * @return number of characters sent, or -1 if error
 */
int llwrite(int fd, unsigned char * buffer, unsigned int length);

/**
 * @param fd data link identifier
 * @param status flag  identifying the current mode (TRANSMITTER | RECEIVER)
 * @return positive number if success, or -1 if error
 */
int llclose(int fd, unsigned int status);

int setBaudRate(unsigned int baud);
int setHeaderErrorRate(int rate);
int setFrameErrorRate(int rate);
int setTimeOut(int to);
int setNumTransm(int n);

#endif
