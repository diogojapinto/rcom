#include "DataLinkProt.h"
#include "macros.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

enum transState {
	INIT_FLAG_ST, ADDR_ST, CTRL_ST, BCC1_ST, DATA_ST, BCC2_ST, END_FLAG_ST
};

linkLayer_t link_props;
unsigned int isInitialized = 0;
unsigned int isOpen = 0;

// structures that save the definitions of the port file
struct termios oldtio, newtio;

/** alarm variables: */
//	set to 0 by the alarm listener, and back to 1 by the application
int alarmFlag = 1;

// counts the number of times the alarm was triggered
unsigned int alarmCounter = 0;

int initLinkProps(unsigned int port);
int openPortFile(unsigned int port);
int sendSetSupFrame(int fd);
struct sigaction installAlarmListener();
int uninstallAlarmListener(struct sigaction old_sa);
int receiveUASupFrame(int fd, unsigned int status);
int closePortFile(int fd);
void alarmListener(int sig);
int receiveSetSupFrame(int fd);
int sendUASupFrame(int fd, unsigned int status);

int genBCC2(unsigned char *buffer, unsigned int length, unsigned char *bcc2);
int byteStuff(unsigned char *buffer, unsigned int length, unsigned char *new_buffer);
int transmitData(int fd, unsigned char *data, unsigned int length);
int recReceiverResp(int fd, unsigned char *data, unsigned int length, unsigned char bcc2);

int byteDestuff(unsigned char *buffer, unsigned int length, unsigned char *new_buffer);
int sendRR(int fd);
int sendREJ(int fd);

int sendDiscSupFrame(int fd, unsigned int status);
int receiveDiscSupFrame(int fd, unsigned int status);

int llopen(unsigned int port, unsigned int status) {

	if (initLinkProps(port))
		return -1;

	link_props.status = status;

	int fd = openPortFile(port);

	if (status == TRANSMITTER) {
		if (sendSetSupFrame(fd))
			return -1;
		if (receiveUASupFrame(fd, status))
			return -1;
	} else if (status == RECEIVER) {
		if (receiveSetSupFrame(fd))
			return -1;
		if (sendUASupFrame(fd, status))
			return -1;
	} else {
		printf("Invalid \"status\" defined! Exiting...");
		closePortFile(fd);
		return -1;
	}
	isInitialized = -1;
	return fd;
}

int llwrite(int fd, unsigned char * buffer, unsigned int length) {
	if (link_props.status == RECEIVER) {
		printf("Invalid call: you cannot write to the serial port!\n");
		return -1;
	}
	if (!link_props.isOpen) {
		printf("Serial port is not initialized. Please run llopen() as TRANSMITTER before calling this function!\n");
		return -1;
	}
	unsigned char bcc2 = 0;
	genBCC2(buffer, length, &bcc2);

	unsigned char tmp_buffer[MAX_FRAME_SIZE] = {0};
	memcpy(tmp_buffer, buffer, length);
	tmp_buffer[length] = bcc2;

	unsigned char *stuffed_buf = malloc(MAX_FRAME_SIZE);
	int stuffed_length = 0;
	if ((stuffed_length = byteStuff(tmp_buffer, length + 1, stuffed_buf)) == -1) {
		printf("Error stuffing data!\n");
		return -1;
	}

	transmitData(fd, stuffed_buf, stuffed_length);
	if (recReceiverResp(fd, stuffed_buf, stuffed_length, bcc2) == -1) {
		printf("recReceiverResp:\n");
		return -1;
	}

	free(stuffed_buf);
	link_props.sequenceNumber = NEXT_DATA_INDEX(link_props.sequenceNumber);
	return 0;
}

int llread(int fd, unsigned char *buffer) {
	if (link_props.status == TRANSMITTER) {
		printf("Invalid call: you cannot read from the serial port!\n");
		return -1;
	}
	if (!link_props.isOpen) {
		printf("Serial port is not initialized. Please run llopen() as RECEIVER before calling this function!\n");
		return -1;
	}
	unsigned char addr = 0;
	unsigned char ctrl = 0;

	unsigned int data_counter = 0;
	unsigned char stuffed_data[MAX_FRAME_SIZE];
	int i = INIT_FLAG_ST;
	int stop = 0;
	while (!stop) {
		unsigned char c = 0;
		if (read(fd, &c, 1)) {
			switch (i) {
			case INIT_FLAG_ST:
				if (c == FLAG) {
					i = ADDR_ST;
				}
				break;
			case ADDR_ST:
				if (c == ADDR_TRANSM) {
					addr = c;
					i = CTRL_ST;
				} else if (c != FLAG) {
					i = INIT_FLAG_ST;
				}
				break;
			case CTRL_ST:
				if (c == GET_CTRL_DATA_INDEX(link_props.sequenceNumber) || c == C_DISC) {
					ctrl = c;
					i = BCC1_ST;
				} else if (c == FLAG) {
					i = ADDR_ST;
				} else {
					i = INIT_FLAG_ST;
				}
				break;
			case BCC1_ST:
				if (c == (addr ^ ctrl)) {
					i = DATA_ST;
				} else if (c == FLAG) {
					i = ADDR_ST;
				} else {
					i = INIT_FLAG_ST;
				}
				break;
			case DATA_ST:
				if (c != FLAG) {
					stuffed_data[data_counter] = c;
					data_counter++;
				} else {
					if (ctrl == C_DISC) {
						// received disconect request
						if (sendDiscSupFrame(fd, RECEIVER))
							return -1;
						if (receiveUASupFrame(fd, RECEIVER))
							return -1;
						link_props.isOpen = 0;
						return DISCONNECTED;
					} else {
						printf("inside else\n");
						// received data packet
						unsigned int data_size = 0;
						if ((data_size = byteDestuff(stuffed_data, data_counter, buffer)) == -1) {
							return -1;
						}

						unsigned char bcc2_rec = buffer[data_size - 1];
						unsigned char bcc2_act = 0;
						genBCC2(buffer, data_size-1, &bcc2_act);
						printf("data_size: %d\n", data_size);
						printf("before bcc2\n");
						if (bcc2_rec == bcc2_act) {
							link_props.sequenceNumber = NEXT_DATA_INDEX(link_props.sequenceNumber);
							sendRR(fd);
							return (data_size-1);
						} else {
							if (ctrl != GET_CTRL_DATA_INDEX(link_props.sequenceNumber)) {
								sendRR(fd);
							} else {
								sendREJ(fd);
							}
							//data_counter = 0; //reset para receber nova trama
						}
					}
				}
				break;
			}
		}
	}
	printf("FRAME RECEIVED\n");

	return 0;
}

int llclose(int fd, unsigned int status) {
	if (!isInitialized) {
		printf("Please first call llopen() to initialize the link layer!\n");
		return -1;
	}

	if (status == TRANSMITTER) {
		if (sendDiscSupFrame(fd, status))
			return -1;
		if (receiveDiscSupFrame(fd, status))
			return -1;
		if (sendUASupFrame(fd, status))
			return -1;
	}

	printf("\nFinishing disconnecting process\n");
	sleep(1);
	if (closePortFile(fd))
		return -1;

	return 0;
}

int initLinkProps(unsigned int port) {
	if (port != 0 && port != 1) {
		printf("Port selected is invalid. Please choose port 0 or 1!\n");
		return -1;
	}
	sprintf(link_props.port, "/dev/ttyS%d", port);

	link_props.baudRate = B38400;
	link_props.sequenceNumber = 0;
	link_props.timeout = 3;
	link_props.numTransmissions = 3;
	link_props.isOpen = -1;

	return 0;
}

int openPortFile(unsigned int port) {

	// variable for initialization of the serial port
	int fd;
	struct termios oldtio, newtio;

	fd = open(link_props.port, O_RDWR | O_NOCTTY);
	if (fd < 0) {
		perror(link_props.port);
		closePortFile(fd);
		return -1;
	}

	if (tcgetattr(fd, &oldtio) == -1) {
		perror("tcgetattr");
		closePortFile(fd);
		return -1;
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = link_props.baudRate | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = OPOST;

	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN] = 1;

	tcflush(fd, TCIFLUSH);

	if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
		perror("tcsetattr");
		closePortFile(fd);
		return -1;
	}
	return fd;
}

int sendSetSupFrame(int fd) {
	unsigned char SET[5];

	SET[0] = FLAG;
	SET[1] = ADDR_TRANSM;
	SET[2] = C_SET;
	SET[3] = (SET[1] ^ SET[2]);
	SET[4] = FLAG;

	write(fd, SET, 5);

	printf("\nSent SET\n");

	return 0;
}

void alarmListener(int sig) {
	printf("alarm # %d\n", alarmCounter);
	alarmFlag = 0;
	alarmCounter++;
}

struct sigaction installAlarmListener() {
	struct sigaction sa, old_sa;
	sa.sa_handler = alarmListener;
	sa.sa_flags = 0;
	sigaction(SIGALRM, &sa, &old_sa);

	return old_sa;
}

int uninstallAlarmListener(struct sigaction old_sa) {
	alarm(0);
	sigaction(SIGALRM, &old_sa, NULL);
	return 0;
}

int receiveUASupFrame(int fd, unsigned int status) {

	// sets the alarm after defining the SIG_ALARM handler
	struct sigaction old_sa = installAlarmListener();
	alarm(link_props.timeout);

	// re-initializes the alarm variables
	alarmFlag = 1;
	alarmCounter = 0;

	unsigned char addr = 0;
	unsigned char ctrl = 0;

	int i = INIT_FLAG_ST;
	int stop = 0;
	while (!stop) {
		unsigned char c = 0;
		if (alarmCounter >= link_props.numTransmissions) {
			printf("Number of tries to receive package exceded. Exiting...\n");
			closePortFile(fd);
			uninstallAlarmListener(old_sa);
			return -1;
		} else if (alarmFlag == 0) {
			if (status == TRANSMITTER) {
				sendSetSupFrame(fd);
			} else if (status == RECEIVER) {
				sendDiscSupFrame(fd, status);
			}
			alarmFlag = 1;
			alarm(link_props.timeout);
		}
		if (read(fd, &c, 1)) {
			switch (i) {
			case INIT_FLAG_ST:
				if (c == FLAG) {
					i = ADDR_ST;
				}
				break;
			case ADDR_ST:
				if ((status == TRANSMITTER && c == ADDR_RECEIV_RESP)
						|| (status == RECEIVER && c == ADDR_TRANSM_RESP)) {
					addr = c;
					i = CTRL_ST;
				} else if (c != FLAG) {
					i = INIT_FLAG_ST;
				}
				break;
			case CTRL_ST:
				if (c == C_UA) {
					ctrl = c;
					i = BCC1_ST;
				} else if (c == FLAG) {
					i = ADDR_ST;
				} else {
					i = INIT_FLAG_ST;
				}
				break;
			case BCC1_ST:
				if (c == (addr ^ ctrl)) {
					i = END_FLAG_ST;
				} else if (c == FLAG) {
					i = ADDR_ST;
				} else {
					i = INIT_FLAG_ST;
				}
				break;
			case END_FLAG_ST:
				if (c == FLAG) {
					stop = -1;
				} else {
					i = INIT_FLAG_ST;
				}
				break;
			}
		}
	}
	printf("\nUA Received!\n");
	uninstallAlarmListener(old_sa);
	return 0;
}

int closePortFile(int fd) {
	sleep(1);
	if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
		perror("tcsetattr");
		return -1;
	}
	close(fd);

	return 0;
}

int receiveSetSupFrame(int fd) {
	int i = INIT_FLAG_ST;
	int stop = 0;
	while (!stop) {
		unsigned char c = 0;
		if (read(fd, &c, 1)) {
			switch (i) {
			case INIT_FLAG_ST:
				if (c == FLAG)
					i = ADDR_ST;
				break;
			case ADDR_ST:
				if (c == ADDR_TRANSM) {
					i = CTRL_ST;
				} else if (c != FLAG) {
					i = INIT_FLAG_ST;
				}
				break;
			case CTRL_ST:
				if (c == C_SET) {
					i = BCC1_ST;

				} else if (c == FLAG) {
					i = ADDR_ST;
				} else {
					i = INIT_FLAG_ST;
				}
				break;
			case BCC1_ST:
				if (c == (ADDR_TRANSM ^ C_SET)) {
					i = END_FLAG_ST;

				} else if (c == FLAG) {
					i = ADDR_ST;
				} else {
					i = INIT_FLAG_ST;
				}
				break;
			case END_FLAG_ST:
				if (c == FLAG) {
					stop = -1;

				} else {
					i = INIT_FLAG_ST;
				}
				break;
			}
		}
	}
	printf("\nSET successfully received\n");
	return 0;
}

int sendUASupFrame(int fd, unsigned int status) {
	unsigned char UA[5];
	UA[0] = FLAG;
	if (status == TRANSMITTER)
		UA[1] = ADDR_TRANSM_RESP;
	else if (status == RECEIVER)
		UA[1] = ADDR_RECEIV_RESP;
	UA[2] = C_UA;
	UA[3] = UA[1] ^ UA[2];
	UA[4] = FLAG;

	write(fd, UA, 5);
	printf("\nSent UA\n");

	return 0;
}

int sendDiscSupFrame(int fd, unsigned int status) {
	unsigned char DISC[5];
	DISC[0] = FLAG;
	if (status == TRANSMITTER)
		DISC[1] = ADDR_TRANSM;
	else if (status == RECEIVER)
		DISC[1] = ADDR_RECEIV_RESP;
	DISC[2] = C_DISC;
	DISC[3] = DISC[1] ^ DISC[2];
	DISC[4] = FLAG;

	write(fd, DISC, 5);
	printf("\nSent DISC\n");
	return 0;
}

int receiveDiscSupFrame(int fd, unsigned int status) {
	struct sigaction old_sa;
	if (status == TRANSMITTER) {
		// sets the alarm after defining the SIG_ALARM handler
		old_sa = installAlarmListener();
		alarm(link_props.timeout);

		// re-initializes the alarm variables
		alarmFlag = 1;
		alarmCounter = 0;
	}

	unsigned char addr = 0;
	unsigned char ctrl = 0;

	int i = INIT_FLAG_ST;
	int stop = 0;
	while (!stop) {
		unsigned char c = 0;
		if (status == TRANSMITTER) {
			if (alarmCounter >= link_props.numTransmissions) {
				printf("Number of tries to receive package exceded. Exiting...\n");
				closePortFile(fd);
				uninstallAlarmListener(old_sa);
				return -1;
			} else if (alarmFlag == 0) {
				sendDiscSupFrame(fd, status);
				alarmFlag = 1;
				alarm(link_props.timeout);
			}
		}

		if (read(fd, &c, 1)) {
			switch (i) {
			case INIT_FLAG_ST:
				if (c == FLAG) {
					i = ADDR_ST;
				} else {
					i = INIT_FLAG_ST;
				}
				break;
			case ADDR_ST:
				if ((status == TRANSMITTER && c == ADDR_RECEIV_RESP)
						|| (status == RECEIVER && c == ADDR_TRANSM)) {
					addr = c;
					i = CTRL_ST;
				} else if (c == INIT_FLAG_ST) {
					i = ADDR_ST;
				}
				break;
			case CTRL_ST:
				if (c == C_DISC) {
					ctrl = c;
					i = BCC1_ST;
				} else if (c == FLAG) {
					i = ADDR_ST;
				} else {
					i = INIT_FLAG_ST;
				}
				break;
			case BCC1_ST:
				if (c == (addr ^ ctrl)) {
					i = END_FLAG_ST;
				} else if (c == FLAG) {
					i = ADDR_ST;
				} else {
					i = INIT_FLAG_ST;
				}
				break;
			case END_FLAG_ST:
				if (c == FLAG) {
					i++;
					stop = -1;
				} else {
					i = INIT_FLAG_ST;
				}
				break;
			}
		}
	}
	printf("\nDISC Received!\n");
	uninstallAlarmListener(old_sa);
	return 0;
}

int genBCC2(unsigned char * buffer, unsigned int length, unsigned char *bcc2) {
	*bcc2 = 0;
	unsigned int i = 0;
	for (; i < length; i++) {
		*bcc2 ^= buffer[i];
	}

	return 0;
}

int byteStuff(unsigned char *buffer, unsigned int length, unsigned char *new_buffer) {

	unsigned int stuff_pos = 0;
	unsigned int i = 0;
	for (; i < length; i++) {
		char c = buffer[i];
		if (c == FLAG || c == ESCAPE_CHAR) {
			new_buffer[stuff_pos++] = ESCAPE_CHAR;
			new_buffer[stuff_pos++] = c ^ DIFF_OCT;
		} else {
			new_buffer[stuff_pos++] = c;
		}
	}
	return stuff_pos;
}

int transmitData(int fd, unsigned char *data, unsigned int length) {

	unsigned char flag = FLAG;
	unsigned char addr = ADDR_TRANSM;
	unsigned char ctrl_data = GET_CTRL_DATA_INDEX(link_props.sequenceNumber);
	unsigned char bcc1 = addr ^ ctrl_data;

	write(fd, &flag, 1);
	write(fd, &addr, 1);
	write(fd, &ctrl_data, 1);
	write(fd, &bcc1, 1);
	write(fd, data, length);
	write(fd, &flag, 1);

	return 0;
}

int recReceiverResp(int fd, unsigned char *data, unsigned int length, unsigned char bcc2) {

	// sets the alarm after defining the SIG_ALARM handler
	struct sigaction old_sa = installAlarmListener();
	alarm(link_props.timeout);

	// re-initializes the alarm variables
	alarmFlag = 1;
	alarmCounter = 0;

	unsigned char addr = 0;
	unsigned char ctrl = 0;

	int i = INIT_FLAG_ST;
	int stop = 0;
	while (!stop) {
		unsigned char c = 0;
		if (alarmCounter >= link_props.numTransmissions) {
			printf("Number of tries to receive package exceded. Exiting...\n");
			closePortFile(fd);
			uninstallAlarmListener(old_sa);
			return -1;
		} else if (alarmFlag == 0) {
			// retransmit data
			transmitData(fd, data, length/*, bcc2*/);
			alarmFlag = 1;
			alarm(link_props.timeout);
		}
		if (read(fd, &c, 1)) {
			switch (i) {
			case INIT_FLAG_ST:
			printf("init\n");
				if (c == FLAG) {
					i = ADDR_ST;
				}
				break;
			case ADDR_ST:
			printf("addr\n");
				if (ADDR_RECEIV_RESP) {
					addr = c;
					i = CTRL_ST;
				} else if (c != FLAG) {
					i = INIT_FLAG_ST;
				}
				break;
			case CTRL_ST:
				printf("ctrl\n");
				if ((c == GET_CTRL_RECEIVER_READY_INDEX(NEXT_DATA_INDEX(link_props.sequenceNumber)))
						|| (c == GET_CTRL_RECEIVER_READY_INDEX(link_props.sequenceNumber))
						|| (c == GET_CTRL_RECEIVER_REJECT_INDEX(link_props.sequenceNumber))) {
					ctrl = c;
					i = BCC1_ST;
				} else if (c == FLAG) {
					i = ADDR_ST;
				} else {
					i = INIT_FLAG_ST;
				}
				break;
			case BCC1_ST:
				printf("bcc1\n");
				if (c == (addr ^ ctrl)) {
					i = END_FLAG_ST;
				} else if (c == FLAG) {
					i = ADDR_ST;
				} else {
					i = INIT_FLAG_ST;
				}
				break;
			case END_FLAG_ST:
				printf("end\n");
				if (c == FLAG) {
					if (ctrl == GET_CTRL_RECEIVER_READY_INDEX(NEXT_DATA_INDEX(link_props.sequenceNumber))) {
						printf("\nReceiver Ready Received!\n");
						stop = -1;
					} else if (ctrl == GET_CTRL_RECEIVER_REJECT_INDEX(link_props.sequenceNumber)) {
						printf("\nReceiver Reject Received!\n");
						transmitData(fd, data, length/*, bcc2*/);
						i = INIT_FLAG_ST;
						alarm(link_props.timeout);
					} else if (ctrl == GET_CTRL_RECEIVER_READY_INDEX(link_props.sequenceNumber)) {
						printf("\nRepeated packet sent. Sending next one.\n");
						stop = -1;
					}
				} else {
					i = INIT_FLAG_ST;
				}
				break;
			}
		}
	}

	uninstallAlarmListener(old_sa);
	return 0;
}

int sendRR(int fd) {
	unsigned char RR[5];
	RR[0] = FLAG;
	RR[1] = ADDR_RECEIV_RESP;
	RR[2] = GET_CTRL_RECEIVER_READY_INDEX(link_props.sequenceNumber);
	RR[3] = RR[1] ^ RR[2];
	RR[4] = FLAG;

	write(fd, RR, 5);
	printf("Sent RR\n");

	return 0;
}

int sendREJ(int fd) {
	unsigned char REJ[5];
	REJ[0] = FLAG;
	REJ[1] = ADDR_RECEIV_RESP;
	REJ[2] = GET_CTRL_RECEIVER_REJECT_INDEX(link_props.sequenceNumber);
	REJ[3] = REJ[1] ^ REJ[2];
	REJ[4] = FLAG;

	write(fd, REJ, 5);
	printf("Sent REJ\n");

	return 0;
}

int byteDestuff(unsigned char *buffer, unsigned int length, unsigned char *new_buffer) {

	unsigned int destuff_pos = 0;
	unsigned int i = 0;
	for (; i < length; i++) {
		char c = buffer[i];
		if (c == ESCAPE_CHAR) {
			c = buffer[++i];
			if (c == (FLAG ^ DIFF_OCT)) {
				new_buffer[destuff_pos++] = FLAG;
			} else if (ESCAPE_CHAR ^ DIFF_OCT) {
				new_buffer[destuff_pos++] = ESCAPE_CHAR;
			} else {
				printf("Error destuffing received data frame.\n");
				return -1;
			}
		} else {
			new_buffer[destuff_pos++] = c;
		}
	}
	return destuff_pos;
}
