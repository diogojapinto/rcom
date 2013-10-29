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
#include <ctype.h>
#include <errno.h>

int initAppProps();
int cliAskStatus();
int cliAskSerialPort();
unsigned char *cliAskDestination();
unsigned char *cliAskSourceFile();
int cliAskMaxPacketSize();
unsigned char *cliAskBaudrate();
int cliAskTimeOut();
int cliAskTransmissions();
int isDirectoryValid(unsigned char *path);
int isFileValid(unsigned char *path);
int openFile(unsigned char *path, int status);
int sendCtrlPacket(int flag);
int receiveCtrlPacket(int flag);
int sendDataPacket(unsigned char *data, unsigned int size);
int processDataPacket(unsigned char *packet);
int writeFile(unsigned char *data, unsigned int oct_number);
int verifyDataIntegrity(unsigned char *buffer, unsigned int size);

applicationLayer_t appProps;

int runApplication() {
	initAppProps();
	appProps.status = cliAskStatus();
	while(appProps.status != EXIT_APP) {
		appProps.filePort = cliAskSerialPort();
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
	printf("\nWaiting for connection!\n");

	if ((appProps.serialPortFileDescriptor = llopen(appProps.filePort, appProps.status)) == -1) {
		free(dest_folder);
		printf("Error opening connection!");
		return -1;
	}

	receiveCtrlPacket(CTRL_START);

	unsigned char file_path[PATH_MAX];
	if (sprintf((char *)file_path, "%s/%s", (char *) dest_folder, appProps.fileName) < 0) {
		printf("sprintf error!\n");
		return -1;
	}

	if ((appProps.fileDescriptor = openFile(file_path, RECEIVER)) == -1) {
		return -1;
	}
	int ret_val = 0;
	int last_buf_size = 0;
	unsigned char buffer[MAX_APP_DATAPACKET_SIZE];
	while(1) {
		ret_val = llread(appProps.serialPortFileDescriptor, buffer);
		if (ret_val == DISCONNECTED) {
			if (verifyDataIntegrity(buffer, last_buf_size) == N_VALID) {
				printf("Errors during transmission. File Invalid!\n");
			}
			break;
		}
		else {
			last_buf_size = ret_val;
			processDataPacket(buffer);
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

	if ((appProps.serialPortFileDescriptor = llopen(appProps.filePort, appProps.status)) == -1) {
		close(appProps.fileDescriptor);
		free(path);
		printf("Error opening connection!");
		return -1;
	}

	if (sendCtrlPacket(CTRL_START) == -1) {
		printf("Error establishing connection to application!\n");
		return -1;
	}
	
	int stop = 0;
	unsigned char *packet = malloc(appProps.dataPacketSize);
	unsigned int size = 0;
	unsigned int i = 0;
	while(!stop) {

		if ((size = read(appProps.fileDescriptor, packet, appProps.dataPacketSize)) != appProps.dataPacketSize) {
			stop = -1;
		}
		if (sendDataPacket(packet, size) == -1) {
			printf("Error transmitting files!\n");
			return -1;
		}
	}

	if (sendCtrlPacket(CTRL_END) == -1) {
		printf("Error closing connection to application!!\n");
		return -1;
	}

	llclose(appProps.serialPortFileDescriptor, appProps.status);

	close(appProps.fileDescriptor);
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
	char tmp[MAX_STRING_SIZE];
	gets((char *)tmp);
	sscanf(tmp, "%d", &c);

	while(c != TRANSMITTER && c != RECEIVER && c != EXIT_APP) {
		printf("\nInvalid input. Please choose a valid one!\n\n");
		gets((char *)tmp);
		sscanf(tmp, "%d", &c);
	}

	return c;
}

int cliAskSerialPort() {
	printf("\nChoose a serial port ('0' - '4')\n\n");

	int c = -1;
	char tmp[MAX_STRING_SIZE];
	gets((char *)tmp);
	sscanf(tmp, "%d", &c);

	while(c < 0 || c > 4) {
		c = -1;
		printf("Invalid input. Please choose a valid serial port number\n");
		gets((char *)tmp);
		sscanf(tmp, "%d", &c);
	}

	return 0;
}

int cliAskMaxPacketSize() {
	unsigned int max_bytes = 0;
	printf("Insert the maximum number of data bytes per frame \n(min value: 1\nmax value: %d)\n", MAX_APP_DATAPACKET_SIZE);
	char tmp[MAX_STRING_SIZE];
	gets((char *)tmp);
	sscanf(tmp, "%u", &max_bytes);

	while(max_bytes < 1 || max_bytes > (MAX_APP_DATAPACKET_SIZE < appProps.fileSize * pow(2,8) ? MAX_APP_DATAPACKET_SIZE : appProps.fileSize * pow(2,8))  ) {
		max_bytes = 0;
		printf("Invalid number of data bytes per frame. Please input another value\n\n");
		gets((char *)tmp);
		sscanf(tmp, "%u", &max_bytes);
	}

	return max_bytes;
}

unsigned char *cliAskDestination() {
	printf("\nWhat is the path to the destination folder of the file?\n\n");

	unsigned char *path = malloc(PATH_MAX);
	memset(path, 0, PATH_MAX);
	gets((char *)path);

	while (isDirectoryValid(path) != 1) {
		memset(path, 0, PATH_MAX);
		printf("\nInvalid folder path. Please input a valid one!\n\n");
		gets((char *)path);
	}

	return path;
}

unsigned char *cliAskBaudrate() {

	unsigned int baud;
	printf("Insert the desired baudrate:\n");
	char tmp[MAX_STRING_SIZE];
	gets((char *)tmp);
	sscanf(tmp, "%d", &baud);

	while(1) {


            
            
            
            
            
            
            
            
            
            B2400
            B4800
            B9600
            B19200
            B38400

		switch(index) {
			case 1:
			setBaudRate(B50)
			break;
			case 2:
			setBaudRate(B75)
			break;
			case 3:
			setBaudRate(B110)
			break;
			case 4:
			setBaudRate(B134)
			break;
			case 5:
			setBaudRate(B150)
			break;
			case 6:
			setBaudRate(B200)
			break;
			case 7:
			setBaudRate(B300)
			break;
			case 8:
			setBaudRate(B600)
			break;
			case 9:
			setBaudRate(B1200)
			break;
			case 10:
			setBaudRate(B1800)
			break;
			case 11:
			setBaudRate(B50)
			break;
			case 12:
			setBaudRate(B50)
			break;
			case 13:
			setBaudRate(B50)
			break;
			case 14:
			setBaudRate(B50)
			break;
			case 15:
			setBaudRate(B50)
			break;
		}
	}

	return max_bytes;
}
int cliAskTimeOut() {

}

int cliAskTransmissions() {

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
	memset(path, 0, PATH_MAX);
	gets((char *)path);

	while (isFileValid(path)) {
		memset(path, 0, PATH_MAX);
		printf("\nInvalid file path. Please input a valid one!\n\n");
		gets((char *)path);
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

int openFile(unsigned char *path, int status) {
	int fd = 0;

	if (status == TRANSMITTER) {
		if ((fd = open((char *) path, O_RDONLY)) == -1) {
			perror("open()");
			return -1;
		}	
	}
	else if (status == RECEIVER) {
		if ((fd = open((char *) path, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)) == -1) {
			
			if (errno == EEXIST) {
				printf("File already exists!\n");
				printf("Do you want to override it?(y/n)\n");
				char opt = 0;
				char tmp[MAX_STRING_SIZE];
				gets((char *)tmp);
				sscanf(tmp, "%c", &opt);
				opt = tolower(opt);
				while (opt != 'y' && opt != 'n' ) {
					opt = 0;
					printf("\nInvalid option. Please input a valid one!\n\n");
					gets((char *)tmp);
					sscanf(tmp, "%c", &opt);				
					opt = tolower(opt);
				}

				if (opt == 'y') {
					if ((fd = open((char *)path, O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)) == -1) {
						perror("open()");
						return -1;
					}
				}
				else {
					return -1;	
				}
			} 
			else {
				perror("open()");
				return -1;
			}
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

	return llwrite(appProps.serialPortFileDescriptor, packet, size);
}

int receiveCtrlPacket(int flag) {
	unsigned char packet[MAX_APP_DATAPACKET_SIZE] = {0};
	int size = 0;
	int i = 0;

	if ((size = llread(appProps.serialPortFileDescriptor, packet)) == -1) {
		return -1;
	}

	if (packet[i] != flag) {
		printf("Error receiving initial packet\n");
		return -1;
	}

	i++;
	while(i < size) {
		

		switch(packet[i]) {
			case T_FILE_SIZE:
			i++;
			memcpy(&appProps.fileSize, &packet[i + 1], packet[i]);
			i += sizeof(unsigned int) + 1;
			break;
			case T_FILE_NAME:
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

	packet[i++] = CTRL_DATA;
	packet[i++] = appProps.currSeqNum++	% 255;

	uint16_t oct_number = size;

	memcpy(&packet[i], &oct_number, 2);
	i+=2;

	memcpy(&packet[i], data, oct_number);

	return llwrite(appProps.serialPortFileDescriptor, packet, (size + 4));
}

int processDataPacket(unsigned char *packet) {

	unsigned int i = 0;
	unsigned int ctrl = 0;
	uint16_t oct_number = 0;
	unsigned int num_seq = 0;
	unsigned char *data;

	ctrl = packet[i++];

	if (ctrl == CTRL_DATA) {

		num_seq = packet[i++];
		memcpy(&oct_number, &packet[i], 2);
		i+=2;

		if ((data = malloc(oct_number+1)) == NULL) {
			printf("Error malloc\n");
			return -1;
		}
		memset(data, 0, oct_number+1);

		memcpy(data, &packet[i], oct_number);
		if (writeFile(data, oct_number) == -1) {
			printf("error writing file\n");
			return -1;
		}

		free(data);
	}

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
	struct stat file_stat;

	ctrl = buffer[i++];

	if (ctrl != CTRL_END) {
		return N_VALID;
	}

	unsigned int file_size = 0;
	unsigned char file_name[MAX_STRING_SIZE] = {0};
	while(i < size) {
		switch(buffer[i]) {
			case T_FILE_SIZE:
			i++;
			memcpy(&file_size, &buffer[i + 1], buffer[i]);
			i += buffer[i] + 1;
			if (file_size != appProps.fileSize) {
				return N_VALID;
			}

			if (fstat(appProps.fileDescriptor, &file_stat) == -1) {
				perror("stat()");
				return N_VALID;
			}

			if (S_ISREG(file_stat.st_mode)) {
				if (file_stat.st_size != file_size) {
					return N_VALID;
				}
			} else {
				return N_VALID;
			}
			break;
			case T_FILE_NAME:
			i++;
			memcpy(&file_name, &buffer[i + 1], buffer[i]);
			i += buffer[i] + 1;
			if (strcmp((char *) file_name, (char *) appProps.fileName) != 0) {
				return N_VALID;
			}
			break;
		}
	}


	return VALID;
}