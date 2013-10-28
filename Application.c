#include "Application.h"
#include "DataLinkProt.h"
#include "macros.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

int initAppProps();
int cliAskStatus();
int cliAskSerialPort();
unsigned char *cliAskDestination();
unsigned char *cliAskSourceFile();
int cliAskMaxPacketSize();
int isDirectoryValid(unsigned char *path);
int isFileValid(unsigned char *path);
int openFile(unsigned char *tmp, int status);
int sendCtrlPacket(int flag);
int receiveCtrlPacket(int flag);
int sendDataPacket(unsigned char *data, unsigned int size);
int processDataPacket(unsigned char *packet);
int writeFile(unsigned char *data, unsigned int oct_number);
int verifyDataIntegrity(unsigned char *buffer, unsigned int size);

applicationLayer_t appProps;

int runApplication() {
	initAppProps();
	appProps.filePort = cliAskSerialPort();
	appProps.status = cliAskStatus();
	while(appProps.status != EXIT_APP) {
		if (appProps.status == TRANSMITTER) {
			sendFile();
		} else {	// is a RECEIVER
			receiveFile();
		}
		initAppProps();
		appProps.status = cliAskStatus();
	} 

	return 0;
}

int receiveFile() {
	unsigned char *dest_folder = cliAskDestination();
	appProps.serialPortFileDescriptor = llopen(appProps.filePort, appProps.status);
	printf("before start\n");

	receiveCtrlPacket(CTRL_START);

	printf("after start\n");
	unsigned char file_path[PATH_MAX];
	if (sprintf((char *)file_path, "%s/%s", (char *) dest_folder, appProps.fileName) < 0) {
		printf("sprintf error!\n");
		return -1;
	}

	printf("%s\n", file_path);

	if ((appProps.fileDescriptor = openFile(file_path, RECEIVER)) == -1) {
		perror("open()");
		return -1;
	}

	int ret_val = 0;
	int last_buf_size = 0;
	unsigned char buffer[MAX_APP_DATAPACKET_SIZE];
	printf("ciclo\n");
	while(1) {
		ret_val = llread(appProps.serialPortFileDescriptor, buffer);
		printf("ret_val: %d\n", ret_val);
		if (ret_val == DISCONNECTED) {
			printf("verify\n");
			if (verifyDataIntegrity(buffer, last_buf_size) == N_VALID) {
				printf("Errors during transmission. File Invalid!\n");
			}
			break;
		}
		else {
			printf("pre process\n");
			last_buf_size = ret_val;
			processDataPacket(buffer);
			printf("processed packet\n");
		}
	}
	
	close(appProps.fileDescriptor);

	llclose(appProps.serialPortFileDescriptor, appProps.status);

	free(dest_folder);

	return 0;
}

int sendFile() {

	unsigned char *path = cliAskSourceFile();
	appProps.fileDescriptor = openFile(path, appProps.status);
	appProps.dataPacketSize = cliAskMaxPacketSize();

	appProps.serialPortFileDescriptor = llopen(appProps.filePort, appProps.status);

	sendCtrlPacket(CTRL_START);
	
	int stop = 0;
	unsigned char *packet = malloc(appProps.dataPacketSize);
	unsigned int size = 0;
	unsigned int i = 0;
	while(!stop) {

		//for (; i < appProps.dataPacketSize; i++) {
		if ((size = read(appProps.fileDescriptor, packet, appProps.dataPacketSize)) != appProps.dataPacketSize) {
			stop = -1;
		}
		printf("data packet size: %d\n", appProps.dataPacketSize);
		printf("size before: %d\n", size);
		int xx = sendDataPacket(packet, size);
		printf("xx: %d\n", xx);
		//}
	}

	sendCtrlPacket(CTRL_END);

	llclose(appProps.serialPortFileDescriptor, appProps.status);

	free(path);
	return 0;
}

int initAppProps() {
	appProps.filePort = -1;
	appProps.serialPortFileDescriptor = 0;
	appProps.status = -1;
	memset(appProps.fileName, 0, PATH_MAX);
	appProps.fileSize = 0;
	appProps.dataPacketSize = 1;
	appProps.currSeqNum = 1;
	appProps.fileDescriptor = 0;

	return 0;
}

int cliAskStatus() {
	printf("\nSelect one (input the number corresponding to the desired option):\n");
	printf("(1) Send file\n");
	printf("(2) Receive file\n");
	printf("(3) Exit application\n\n");

	int c = -1;
	scanf("%d", &c);

	while(c != TRANSMITTER && c != RECEIVER && c != EXIT_APP) {
		printf("\nInvalid input. Please choose a valid one!\n\n");
		scanf("%d", &c);
	}

	return c;
}

int cliAskSerialPort() {
	printf("\nChoose a serial port ('0' - '4')\n\n");

	int c = -1;
	scanf("%d", &c);

	while(c < 0 || c > 4) {
		printf("Invalid input. Please choose a valid serial port number\n");
		scanf("%d", &c);
	}

	return 0;
}

int cliAskMaxPacketSize() {
	unsigned int max_bytes = 0;
	printf("Insert the maximum number of data bytes per frame \n(min value: 1\nmax value: %d)\n", MAX_APP_DATAPACKET_SIZE);
	scanf("%u", &max_bytes);

	while(max_bytes < 1 || max_bytes > (MAX_APP_DATAPACKET_SIZE < appProps.fileSize * pow(2,8) ? MAX_APP_DATAPACKET_SIZE : appProps.fileSize * pow(2,8))  ) {
		printf("Invalid number of data bytes per frame. Please input another value\n\n");
		scanf("%u", &max_bytes);
	}

	return max_bytes;
}

unsigned char *cliAskDestination() {
	printf("\nWhat is the path to the destination folder of the file?\n\n");

	unsigned char *path = malloc(PATH_MAX);
	scanf("%s", path);

	while (isDirectoryValid(path) != 1) {
		printf("\nInvalid folder path. Please input a valid one!\n\n");
		scanf("%s", path);
	}

	return path;
}

int isDirectoryValid(unsigned char *path) {
	int dir_exists = mkdir((char *) path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IROTH);

	struct stat dir_stat;

	if (dir_exists) {
		if (stat((char *) path, &dir_stat) == -1) {
			perror("stat()");
			return -1;
		}

		if (!S_ISDIR(dir_stat.st_mode)) {
			if (strcpy((char *) path, dirname((char *) path)) == NULL) {
				perror("dirname()");
				return -1;
			}
		}
	}
	return 1;
}

unsigned char *cliAskSourceFile() {
	printf("\nWhat is the path to the source file to be copied?\n\n");

	unsigned char *path = malloc(PATH_MAX);
	scanf("%s", path);

	while (isFileValid(path)) {
		printf("\nInvalid file path. Please input a valid one!\n\n");
		scanf("%s", path);
	}

	return path;
}

int isFileValid(unsigned char *path) {
	//unsigned char tmp[PATH_MAX];
	struct stat file_stat;
/*
	if (realpath(path, tmp) == NULL) {
		perror("realpath()");
		return -1;
	}

	strcpy((char *) path, (char *) tmp);
*/
	if (stat((char *) path, &file_stat) == -1) {
		perror("stat()");
		return -1;
	}

	if (S_ISREG(file_stat.st_mode)) {
		strcpy((char *) appProps.fileName, basename((char *)path));
		appProps.fileSize = file_stat.st_size;
		return 0;
	} else {
		return -1;
	}
}

int openFile(unsigned char *tmp, int status) {
	int fd = 0;

	if (status == TRANSMITTER) {
		if ((fd = open((char *) tmp, O_RDONLY)) == -1) {
			perror("open()");
			return -1;
		}	
	}
	else if (status == RECEIVER) {
		if ((fd = open((char *) tmp, O_CREAT | O_EXCL | O_WRONLY)) == -1) {
			perror("open()");
			return -1;
		}
	}
	else {
		return -1;
	}

	return fd;
}

int sendCtrlPacket(int flag) {
	unsigned char packet[MAX_APP_DATAPACKET_SIZE] = {0};
	unsigned int size = 0;

	// add ctrl
	packet[size] = flag;
	size++;

	// add the length of the file
	uint8_t type = T_FILE_SIZE;
	packet[size++] = type;
	uint8_t length = sizeof(unsigned int);
	packet[size++] = length;
	memcpy(&packet[size], &appProps.fileSize, sizeof(unsigned int));
	size += sizeof(unsigned int);

	// add filename
	type = T_FILE_NAME;
	packet[size++] = type;
	length = strlen((char *) appProps.fileName) + 1;
	packet[size++] = length;
	memcpy(&packet[size], &appProps.fileName, strlen((char *) appProps.fileName) + 1);
	//strcat((char *) packet, (char *) appProps.fileName);
	size += strlen((char *) appProps.fileName) + 1;

	int a=0;
	for (a=0; a < size; a++) {
		printf("%X\n", packet[a]);
	}
	printf("%d\n", size);

	return llwrite(appProps.serialPortFileDescriptor, packet, size);
}

int receiveCtrlPacket(int flag) {
	unsigned char packet[MAX_APP_DATAPACKET_SIZE] = {0};
	int size = 0;
	int i = 0;

	if ((size = llread(appProps.serialPortFileDescriptor, packet)) == -1) {
		return -1;
	}

	int a=0;
	for (a=0; a < size; a++) {
		printf("%X\n", packet[a]);
	}

	printf("after read\n");

	if (packet[i] != flag) {
		printf("Error receiving initial packet\n");
		return -1;
	}

	i++;
	printf("%d\n", i);
	printf("%d\n", size);
	printf("before while\n");
	printf("%X\n", packet[i]);
	while(i < size) {
		

		switch(packet[i]) {
			case T_FILE_SIZE:
			printf("file size\n");
			i++;
			memcpy(&appProps.fileSize, &packet[i + 1], packet[i]);
			i += sizeof(unsigned int) + 1;
			break;
			case T_FILE_NAME:
			printf("file name\n");
			i++;
			memcpy(&appProps.fileName, &packet[i + 1], packet[i]);
			i += strlen((char *) appProps.fileName) + 2;
			break;
		}
	}
	return 0;
}

int sendDataPacket(unsigned char *data, unsigned int size) {
	unsigned char packet[MAX_APP_DATAPACKET_SIZE];
	unsigned int i = 0;
	
	printf("%d\n", size);

	packet[i++] = CTRL_DATA;
	packet[i++] = appProps.currSeqNum++	;

	uint16_t oct_number = size;
	printf("oct_number send: %d\n", oct_number);

	memcpy(&packet[i], &oct_number, 2);
	i+=2;

	memcpy(&packet[i], data, oct_number);

	printf("size writen: %d\n", size+4);

	return llwrite(appProps.serialPortFileDescriptor, packet, (size + 4));
}

int processDataPacket(unsigned char *packet) {

	unsigned int i = 0;
	unsigned int ctrl = 0;
	uint16_t oct_number = 0;
	unsigned int num_seq = 0;
	unsigned char *data = malloc(appProps.dataPacketSize);
	
	printf("bfore memset\n");

	memset(data, 0, appProps.dataPacketSize);

	ctrl = packet[i++];
	
	printf("before if\n\n");

	num_seq = packet[i++];

	if (ctrl == CTRL_DATA) {

		printf("memcpy 1\n");
		memcpy(&oct_number, &packet[i], 2);
		printf("oct_number: %d\n", oct_number);
		i+=2;

		printf("memcpy 2\n");
		memcpy(&data, &packet[i], oct_number);
		printf("write\n");
		if (writeFile(data, oct_number) == -1) {
			printf("erro writing file\n");
			return -1;
		}
		printf("write out\n");
	}
	printf("return\n");
	return 0;
}

int writeFile(unsigned char *data, unsigned int oct_number) {


	if (write(appProps.fileDescriptor, data, oct_number) != oct_number) {
		return -1;
	}

	return oct_number;
}

int verifyDataIntegrity(unsigned char *buffer, unsigned int size) {

	unsigned int i = 0;
	unsigned int ctrl = 0;

	ctrl = buffer[i++];

	if (ctrl != CTRL_END) {
		return N_VALID;
	}

	while(i < size) {
		switch(buffer[i]) {
			case T_FILE_SIZE:
			i++;
			unsigned int file_size = 0;
			memcpy(&file_size, &buffer[i + 1], buffer[i]);
			i += sizeof(unsigned int) + 1;
			if (file_size != appProps.fileSize) {
				return N_VALID;
			}
			break;
			case T_FILE_NAME:
			i++;
			unsigned char file_name[MAX_STRING_SIZE] = {0};
			memcpy(&file_name, &buffer[i + 1], buffer[i]);
			i += strlen((char *) file_name) + 1;
			if (strcmp((char *) file_name, (char *) appProps.fileName) != 0) {
				return N_VALID;
			}
			break;
		}
	}
	return VALID;
}