FLAGS= -Wall -pthread

all: download

download: download.o
	gcc $(FLAGS) download.o -o download
	
download.o: download.c
	gcc $(FLAGS) -c download.c
	
clean:
	rm download.o download