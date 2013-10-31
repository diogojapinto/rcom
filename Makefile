# macros definitions
CC = gcc
CFLAGS = -Wall -pthread -lrt -lm
XHDRS = macros.h

all: rcom_proj

rcom_proj: DataLinkProt.o Application.o main.c $(XHDRS) 
	$(CC) DataLinkProt.o Application.o main.c $(XHDRS)  -o $@ $(CFLAGS)

Application.o: Application.c $(XHDRS)
	$(CC) -c Application.h Application.c $(XHDRS) $(CFLAGS)

DataLinkProt.o: DataLinkProt.c $(XHDRS)
	$(CC) -c DataLinkProt.h DataLinkProt.c $(XHDRS) $(CFLAGS)

log.o: log.c $(XHDRS)
	$(CC) -c log.h log.c $(XHDRS) $(CFLAGS)

clean:
	rm -f *.gch *.o rcom_proj