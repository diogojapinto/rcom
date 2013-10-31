#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "macros.h"
#include <limits.h>

struct applicationLayer {
	int filePort;
	int serialPortFileDescriptor;
	int status; /*TRANSMITTER | RECEIVER*/
	int dataPacketSize;
	unsigned char currSeqNum;
	unsigned char fileName[PATH_MAX];
	unsigned int fileSize;
	int fileDescriptor;
}typedef applicationLayer_t;

int runApplication();
int receiveFile();
int sendFile();

#endif
