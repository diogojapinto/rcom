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
 	srand(time(NULL));
 	runApplication();
 	return 0;
 }
