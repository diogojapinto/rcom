/**
 (in DataLinkProt.c)
 the disconect by the receiver is supposed to have the address addr_receiv_resp or addr_receiv ????

 tam max e min de informacao (possibilidade de link layer subdividir os pacotes de dados enviados pela aplicacao)
 2^16

 ficheiro nunca maior que 2^24

 */

#include "DataLinkProt.h"
#include "Application.h"
#include "macros.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

 int main(int argc, char **argv) {
 	/*int fd = 0;

 	if (argc > 1) {
 		if ((fd = llopen(0, TRANSMITTER)) == -1)
 			return -1;
 		if (llwrite(fd, (unsigned char *)"o spaces e fodido do cerebro por estar sempre a fazer breeeding\n", 65) == -1) {
 			return -1;
 		}
 		if (llclose(fd, TRANSMITTER))
 			return -1;
 	} else {
 		if ((fd = llopen(0, RECEIVER)) == -1)
 			return -1;
 		unsigned char tmp[1000];
 		int error = 0;
 		while ((error = llread(fd, tmp)) != DISCONNECTED) {
 			if (error == -1) {
 				return -1;
 			}
 		}
 		printf("%s", tmp);
 		if (llclose(fd, RECEIVER))
 			return -1;
 	}*/

 	runApplication();
 	return 0;
 }
