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
<<<<<<< HEAD
	unsigned int headerErrorRate;
	unsigned int frameErrorRate;
=======
>>>>>>> 590dbd115515b7c899dd3cd1644d93e8dff3b706
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

<<<<<<< HEAD
int setBaudRate(unsigned int baud);
int setHeaderErrorRate(int rate);
int setFrameErrorRate(int rate);
int setTimeOut(int to);
int setNumTransm(int n);

=======
>>>>>>> 590dbd115515b7c899dd3cd1644d93e8dff3b706
#endif
