#include <stdio.h>
#include <stdlib.h>
#include <macros.h>

int writeToLog(int fd, int event, int argc, char ** argv){
	char when[MAX_STRING_SIZE];
	getCurrentTime(when);

	sprintf(line,"%s | %s | %s \n", when_final, what_final, number);

	if (open("serialport.log", O_CREATE | O_APPEND, 0664)) {
		return -1;
	}

	APP_STARTED, APP_STATUS_DEF, APP_SERP_DEF, APP_BAUDRATE_DEF, APP_TIMEOUT_DEF, APP_NUMRETRANSM_DEF, APP_SOURCEF_DEF, APP_MAXPACKSIZE_DEF, 
	DL_SENDING_SET, DL_FAILED_SET, DL_RECEIVED_UA, DL_TIMEOUT, APP_SENDING_STARTPACKET, APP_FAILED, APP_SENDING_BYTES, APP_SENDING_BYTES, 
	DL_ERROR_STUFFING_DATA, DL_SENDING_DATA, DL_RECEIVED_CURR_RR, DL_RECEIVED_NEXT_RR, DL_RECEIVED_REJ, DL_SEND_DISC, DL_REC_DISC, DL_SEND_UA,
	APP_SENDING_FINALPACKET, APP_CLOSING_SER_PORT, APP_CLOSING_FILE, APP_T_SUCCESS, APP_T_FAILURE, APP_SET_HER, APP_SET_FER, APP_SET_DESTFOLD, APP_RECEIV_INITPACK,
	APP_OPENED_DESTFILE, DL_DATA_VERIFIED, DL_HER_GEN, DL_FER_GEN, DL_SENDING_REJ, DL_SEND_CURR_RR, DL_SEND_NEXT_RR, APP_DEST_FILE_DELETED, APP_WRITE_FILE, APP_FILE_NONVALID
	switch(event) {

	}


	return 0;
}

int getCurrentTime(char *buff){
	time_t t = time(NULL);
	struct tm timer = *localtime(&t);
	sprintf(buff,"%04d-%02d-%02d %02d:%02d:%02d", timer.tm_year+1900, timer.tm_mon, timer.tm_mday, timer.tm_hour, timer.tm_min, timer.tm_sec);
	return 0;
}