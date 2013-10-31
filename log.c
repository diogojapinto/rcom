#include <stdio.h>
#include <stdlib.h>
#include <termios.h>

int bauds_m[] = {B50,B75,B110,B134,B150,B200,B300,B600,B1200,B1800,B2400,B4800,B9600,B19200,B38400};
int bauds_i[] = {"none", "50","75","110","134","150","200","300","600","1200","1800","2400","4800","9600","19200","38400"};

void *writeToLog(log_info_t *info) {
	int fd = 0;
	char when[MAX_STRING_SIZE];
	getCurrentTime(when);
	getCurrentTime(who);

	if ((fd = open("serialport.log", O_CREATE | O_APPEND, 0664)) == -1) {
		return -1;
	}

	switch(info.event) {
		APP_STARTED:
		strcpy(who, "application");
		strcpy(what, "aplication started");
		break;
		case APP_STATUS_DEF:
		strcpy(who, "application");
		strcpy(what, "defined as ");
		switch(info.arg) {
			case TRANSMITTER:
			strcat(what, "transmitter");
			break;
			case RECEIVER:
			strcat(what, "receiver");
			break;
		}
		break;
		case APP_SERP_DEF:
		strcpy(who, "application");
		sprintf(what, "serial port defined as /dev/ttyS%d", info.argi);
		break;
		case APP_BAUDRATE_DEF:
		strcpy(who, "application");
		sprintf(what, "baudrate defined as %d", baud_i[info.argi]);
		break;
		case APP_TIMEOUT_DEF:
		strcpy(who, "application");
		strcpy(what, "timeout defined as %d", info.argi);
		break;
		case APP_NUMRETRANSM_DEF:
		strcpy(who, "application");
		strcpy(what, "number of retransmitions set to %d", info.argi);
		break;
		case APP_SOURCEF_DEF:
		strcpy(who, "application");
		strcpy(what, "source file: %s", info.args);
		break;
		case APP_MAXPACKSIZE_DEF:
		strcpy(who, "application");
		strcpy(what, "maximum packet size defined as %d", info.argi);
		break;
		case DL_SENDING_SET:
		strcpy(who, "application");
		strcpy(what, "sending SET supervision frame");
		break;
		case DL_FAILED_SET:
		strcpy(who, "application");
		strcpy(what, "failed sending SET supervision frame");
		break;
		case DL_RECEIVED_UA:
		strcpy(who, "application");
		strcpy(what, "received UA supervision frame");
		break;
		case DL_TIMEOUT:
		strcpy(who, "application");
		strcpy(what, "timout occurred");
		break;
		case APP_SENDING_STARTPACKET:
		strcpy(who, "application");
		strcpy(what, "sending application startup control packet");
		break;
		case APP_FAILED:
		strcpy(who, "application");
		strcpy(what, "application failed");
		break;
		case APP_SENDING_BYTES:
		strcpy(who, "application");
		strcpy(what, "sending %d bytes");
		break;
		case APP_SENDING_BYTES:
		strcpy(who, "application");
		strcpy(what, "sending %d bytes");
		break;
		case DL_ERROR_STUFFING_DATA:
		strcpy(who, "data link");
		strcpy(what, "error stuffing data");
		break;
		case DL_SENDING_DATA:
		strcpy(who, "data link");
		strcpy(what, "sending frame");
		break;
		case DL_RECEIVED_CURR_RR:
		strcpy(who, "data link");
		strcpy(what, "received RR indicating a duplicate... resending");
		break;
		case DL_RECEIVED_NEXT_RR:
		strcpy(who, "data link");
		strcpy(what, "received RR indicating success");
		break;
		case DL_RECEIVED_REJ:
		strcpy(who, "data link");
		strcpy(what, "received REJ");
		break;
		case DL_SEND_DISC:
		strcpy(who, "data link");
		strcpy(what, "sending DISC supervision frame");
		break;
		case DL_REC_DISC:
		strcpy(who, "data link");
		strcpy(what, "received DISC supervision frame");
		break;
		case DL_SEND_UA:
		strcpy(who, "data link");
		strcpy(what, "sending UA");
		break;
		case APP_SENDING_FINALPACKET:
		strcpy(who, "application");
		strcpy(what, "sending application startup control packet");
		break;
		case APP_CLOSING_SER_PORT:
		strcpy(who, "application");
		sprintf(what, "closing serial port at /dev/ttyS%d", info.argi);
		break;
		case APP_CLOSING_FILE:
		strcpy(who, "application");
		strcpy(what, "closing file");
		break;
		case APP_T_SUCCESS:
		strcpy(who, "application");
		strcpy(what, "success transmitting file");
		break;
		case APP_T_FAILURE:
		strcpy(who, "application");
		strcpy(what, "failure receiving file");
		break;
		case APP_SET_HER:
		strcpy(who, "application");
		strcpy(what, "header error rate defined to %d%%", info.argi);
		break;
		case APP_SET_FER:
		strcpy(who, "application");
		strcpy(what, "frame error rate defined to %d%%", info.argi);
		break;
		case APP_SET_DESTFOLD:
		strcpy(who, "application");
		strcpy(what, "destination folder set: %s", info.args);
		break;
		case APP_RECEIV_INITPACK:
		strcpy(who, "application");
		strcpy(what, "received application startup control packet");
		break;
		case APP_OPENED_DESTFILE:
		strcpy(who, "application");
		strcpy(what, "destination file %s openend");
		break;
		case DL_DATA_VERIFIED:
		strcpy(who, "data link");
		strcpy(what, "data verification returned success");
		break;
		case DL_HER_GEN:
		strcpy(who, "data link");
		strcpy(what, "header error generated automatically");
		break;
		case DL_FER_GEN:
		strcpy(who, "data link");
		strcpy(what, "frame error generated automatically");
		break;
		case DL_SENDING_REJ:
		strcpy(who, "data link");
		strcpy(what, "sending REJ");
		break;
		case DL_SEND_CURR_RR:
		strcpy(who, "data link");
		strcpy(what, "receved duplicate... sending RR indicating that");
		break;
		case DL_SEND_NEXT_RR:
		strcpy(who, "data link");
		strcpy(what, "received successfully... sending RR");
		break;
		case APP_DEST_FILE_DELETED:
		strcpy(who, "application");
		strcpy(what, "destination file deleted due to error");
		break;
		case APP_WRITE_FILE:
		strcpy(who, "application");
		strcpy(what, "data writen to destination file");
		break;
		case APP_FILE_NONVALID:
		strcpy(who, "application");
		strcpy(what, "file transmitted with errors");
		break;
	}

	sprintf(line,"\n%-20s|%-10s|%s", when, who, what);
	write(fd, line, strlen(line));

	return 0;
}

int getCurrentTime(char *buff){
	time_t t = time(NULL);
	struct tm timer = *localtime(&t);
	sprintf(buff,"%04d-%02d-%02d %02d:%02d:%02d", timer.tm_year+1900,timer.tm_mon,timer.tm_mday,timer.tm_hour, timer.tm_min, timer.tm_sec);
	return 0;
}