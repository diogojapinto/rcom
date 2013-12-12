#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>

#define MAX_STR_LEN 256
#define WITH_AUTENTICATION 0
#define WITHOUT_AUTENTICATION 1

#define OPEN_FILE_FAILURE 550
#define OPEN_FILE_SUCCESS 150
#define ENTERED_PASSIVE_MODE 227
#define STARTED_CONNECTION 220
#define INPUT_PASSWORD 331
#define LOGGIN_SUCCESS 230
#define TRANSFER_COMPLETE 226

#define FTP_SERVER_PORT 21

typedef struct session {
	char user[MAX_STR_LEN];
	char pass[MAX_STR_LEN];
} session_t;

typedef struct saveFileInfo {
	int fileSize;
	int dest_sockfd;
	char file_name[MAX_STR_LEN];
} saveFileInfo_t;

struct hostent *fillHostInfo(char host[]);

int connectToPort(struct hostent *host, int port);
int loggin(int sockfd, session_t *sess);
int retrDestPort(int srcSocket);
int sendFileRequest(int srcSocket, char *urlPath);
int verifyTransfer(int srcSocket);
void *saveToFile(void *ptr);
int closePort(int sockfd);

int main(int argc, char *argv[]) {

	if (argc < 2) {
		printf("Usage: download <URL-in-RFC1738-syntax>\n");
		return -1;
	}

	session_t session;
	int method;
	char host[MAX_STR_LEN];
	char url_path[MAX_STR_LEN];

	if (sscanf(argv[1], "ftp://%[^:]:%[^@]@%[^/]/%s", session.user,
			session.pass, host, url_path) == 4) {
		method = WITH_AUTENTICATION;
	} else if (sscanf(argv[1], "ftp://%[^/]/%s", host, url_path) == 2) {
		method = WITHOUT_AUTENTICATION;
	} else {
		printf(
				"Invalid URL!\nUsage: ftp://[<user>:<password>@]<host>/<url-path>\n");
		exit(-1);
	}

	if (method == WITH_AUTENTICATION) {
		printf("%-20s: %s\n", "Username", session.user);
		printf("%-20s: %s\n", "Password", session.pass);
	}
	printf("%-20s: %s\n", "Host Name", host);
	printf("%-20s: %s\n\n", "Url Path", url_path);

	struct hostent *host_info = fillHostInfo(host);

	int src_sockfd = connectToPort(host_info, FTP_SERVER_PORT);

	if (method == WITH_AUTENTICATION) {
		loggin(src_sockfd, &session);
	}

	int dest_port = retrDestPort(src_sockfd);

	int dest_sockfd = connectToPort(host_info, dest_port);

	saveFileInfo_t save_info;
	save_info.dest_sockfd = dest_sockfd;
	strcpy(save_info.file_name, basename(url_path));

	save_info.fileSize = sendFileRequest(src_sockfd, url_path);

	pthread_t tid;

	if (pthread_create(&tid, NULL, saveToFile, &save_info) < 0) {
		perror("pthread_create");
		exit(-1);
	}

	verifyTransfer(src_sockfd);

	pthread_join(tid, NULL);

	closePort(src_sockfd);
	closePort(dest_sockfd);

	return 0;
}

struct hostent *fillHostInfo(char host[]) {

	struct hostent *h;

	/*
	 struct hostent {
	 char    *h_name;	Official name of the host.
	 char    **h_aliases;	A NULL-terminated array of alternate names for the host.
	 int     h_addrtype;	The type of address being returned; usually AF_INET.
	 int     h_length;	The length of the address in bytes.
	 char    **h_addr_list;	A zero-terminated array of network addresses for the host.
	 Host addresses are in Network Byte Order.
	 };

	 #define h_addr h_addr_list[0]	The first address in h_addr_list.
	 */

	if ((h = gethostbyname(host)) == NULL) {
		herror("gethostbyname");
		exit(-1);
	}

	printf("%-20s: %s\n", "Official Host Name", h->h_name);
	printf("%-20s: %s\n\n", "IP Address",
			inet_ntoa(*((struct in_addr *) h->h_addr)));

	return h;
}

int connectToPort(struct hostent *host, int port) {
	int sockfd;
	struct sockaddr_in server_addr;

	/*server address handling*/
	bzero((char*) &server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	inet_aton(inet_ntoa(*((struct in_addr *) host->h_addr)), &server_addr.sin_addr); /*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(port); /*server FTP port must be network byte ordered */

	/*open an FTP socket*/
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket()");
		exit(0);
	}

	/*connect to the server*/
	if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr))
			< 0) {
		perror("connect()");
		exit(0);
	}

	if (port == FTP_SERVER_PORT) {
		char resp[MAX_STR_LEN];
		int i = 0;

		do {
			read(sockfd, &resp[i++], 1);
		} while (i < MAX_STR_LEN && resp[i - 1] != '\n');

		resp[i] = '\0';

		int mess_code = 0;
		sscanf(resp, "%d %*[^\n]\n", &mess_code);

		printf("%-20s: %s\n\n", "Message received", resp);

		if (mess_code != STARTED_CONNECTION) {
			printf("Error opening connection!\n");
			exit(-1);
		}
	}

	printf("Connection successfull\n");
	return sockfd;
}

int loggin(int sockfd, session_t *sess) {

	char messg[MAX_STR_LEN];
	char resp[MAX_STR_LEN];
	int i = 0;

	sprintf(messg, "user %s\n", sess->user);
	printf("%-20s: %s\n", "Message sent", messg);
	write(sockfd, messg, strlen(messg));

	do {
		read(sockfd, &resp[i++], 1);
	} while (i < MAX_STR_LEN && resp[i - 1] != '\n');

	resp[i] = '\0';

	printf("%-20s: %s\n", "Message received", resp);

	int mess_code = 0;
	sscanf(resp, "%d %*[^\n]\n", &mess_code);

	if (mess_code != INPUT_PASSWORD) {
		printf("Error logging in!\nPassword not prompted\n");
		exit(-1);
	}

	sprintf(messg, "pass %s\n", sess->pass);
	printf("%-20s: %s\n", "Message sent", messg);
	write(sockfd, messg, strlen(messg));

	i = 0;
	do {
		read(sockfd, &resp[i++], 1);
	} while (i < MAX_STR_LEN && resp[i - 1] != '\n');

	resp[i] = '\0';

	printf("%-20s: %s\n\n", "Message received", resp);

	mess_code = 0;
	sscanf(resp, "%d %*[^\n]\n", &mess_code);

	if (mess_code != LOGGIN_SUCCESS) {
		printf("Error logging in!\n");
		exit(-1);
	}

	printf("Login successful\n\n");

	return 0;
}

int retrDestPort(int srcSocket) {
	char messg[MAX_STR_LEN];
	char resp[MAX_STR_LEN];
	int i = 0;

	strcpy(messg, "pasv\n");
	printf("%-20s: %s\n", "Message sent", messg);
	write(srcSocket, messg, strlen(messg));

	do {
		read(srcSocket, &resp[i++], 1);
	} while (i < MAX_STR_LEN && resp[i - 1] != '\n');

	resp[i] = '\0';

	printf("%-20s: %s\n\n", "Message received", resp);

	int sup_byte = 0;
	int low_byte = 0;

	sscanf(resp, "%*[^(](%*d,%*d,%*d,%*d,%d,%d)\n", &sup_byte, &low_byte);

	int dest_port = (sup_byte << 8) + low_byte;

	printf("%-20s: %d\n", "Destination port", dest_port);

	int mess_code = 0;
	sscanf(resp, "%d %*[^\n]\n", &mess_code);

	if (mess_code != ENTERED_PASSIVE_MODE) {
		printf("Error entering passive mode!\n");
		exit(-1);
	}

	printf("Entered passive mode\n\n");

	return dest_port;
}

int sendFileRequest(int srcSocket, char *urlPath) {

	int file_size = 0;
	char messg[MAX_STR_LEN];
	char resp[MAX_STR_LEN];
	int i = 0;

	sprintf(messg, "retr %s\n", urlPath);
	printf("%-20s: %s\n", "Message sent", messg);
	write(srcSocket, messg, strlen(messg));

	do {
		read(srcSocket, &resp[i++], 1);
	} while (i < MAX_STR_LEN && resp[i - 1] != '\n');

	resp[i] = '\0';

	printf("%-20s: %s\n\n", "Message received", resp);

	int mess_code = 0;
	sscanf(resp, "%d%*[^(](%d%*[^\n]\n", &mess_code, &file_size);

	if (mess_code == OPEN_FILE_FAILURE) {
		printf("Error opening file!\n");
		exit(-1);
	}

	printf("%d bytes sent\n\n", file_size);

	return file_size;
}

int verifyTransfer(int srcSocket) {
	char resp[MAX_STR_LEN];
	int i = 0;
	do {
		read(srcSocket, &resp[i++], 1);
	} while (i < MAX_STR_LEN && resp[i - 1] != '\n');

	resp[i] = '\0';

	printf("%-20s: %s\n\n", "Message received", resp);

	int mess_code = 0;
	sscanf(resp, "%d %*[^\n]\n", &mess_code);

	if (mess_code != TRANSFER_COMPLETE) {
		printf("Error transmitting file!\n");
		exit(-1);
	}

	return 0;
}

void *saveToFile(void *ptr) {

	saveFileInfo_t *save_info = ptr;

	int dest_fd = -1;

	char cwd[MAX_STR_LEN];
	getcwd(cwd, MAX_STR_LEN);
	printf("%-20s: %s/%s\n", "File to be saved as", cwd, save_info->file_name);
	if ((dest_fd = open(save_info->file_name, O_WRONLY | O_CREAT | O_TRUNC,
			0664)) < 0) {
		perror("open");
		exit(-1);
	}

	char c;

	printf("Waiting for file\n\n");

	int i = 0;
	do {
		read(save_info->dest_sockfd, &c, 1);
		write(dest_fd, &c, 1);
		i++;
	} while (i < save_info->fileSize);

	close(dest_fd);

	printf("File saved successfully\n\n");

	return NULL;
}

int closePort(int sockfd) {
	close(sockfd);
	return 0;
}
