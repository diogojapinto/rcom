#ifndef MACROS_H_
#define MACROS_H_

#define MAX_APP_DATAPACKET_SIZE 65536
#define MAX_FRAME_SIZE 131072
#define MAX_STRING_SIZE 10000

/*
 * data link macros
 *
 */
#define _POSIX_SOURCE 1 /* POSIX compliant source */

#define FALSE 0
#define TRUE 1

#define FLAG 0x7E
#define ADDR_TRANSM 0x03
#define ADDR_RECEIV_RESP 0x03
#define ADDR_RECEIV 0x01
#define ADDR_TRANSM_RESP 0x01
#define C_SET 0x03
#define C_UA 0x07
#define C_DISC 0X0B

#define NEXT_DATA_INDEX(SEQ_NUM) (SEQ_NUM ^ 0x01)

#define GET_CTRL_DATA_INDEX(SEQ_NUM) (SEQ_NUM << 1)

#define GET_CTRL_RECEIVER_READY_INDEX(SEQ_NUM) ((SEQ_NUM << 5) | 0x05)

#define GET_CTRL_RECEIVER_REJECT_INDEX(SEQ_NUM) ((SEQ_NUM << 5) | 0x01)

#define ESCAPE_CHAR 0x7D
#define DIFF_OCT 0x20

/*
 * application macros
 */
#define TRANSMITTER 1
#define RECEIVER 2
#define EXIT_APP 3
#define DISCONNECTED -75
#define CTRL_DATA 0
#define CTRL_START 1
#define CTRL_END 2
#define N_VALID 0
#define VALID -1
#define T_FILE_SIZE 0
#define T_FILE_NAME 1

#endif
