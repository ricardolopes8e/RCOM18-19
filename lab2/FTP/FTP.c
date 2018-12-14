#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <strings.h>
#include <ctype.h>

#define SERVER_PORT 21
#define SERVER_ADDR "192.168.28.96"
#define MAX_STRING_LEN 50
#define SOCKET_BUF_SIZE 1000
#define MAX_PORT_LEN 10

typedef struct {
	char user[MAX_STRING_LEN];
	char pass[MAX_STRING_LEN];
	char host[MAX_STRING_LEN];
	char path[MAX_STRING_LEN];
	char filename[MAX_STRING_LEN];
} urlStruct;

void readServerResponse(int socketfd, char *responseCode);
struct hostent *getip(char host[]);
void parseInputArgument(char *inputString, urlStruct **url_info);
int sendCommand(int socketfd, char cmd[], char commandContent[], char* filename, int socketfdClient);
int getServerPort(int socketfd);
void createFile(int fd, char* filename);
void infoStruct(char * urlstr, urlStruct * info);

void infoStruct(char * urlstr, urlStruct * info){

	int n = sscanf(urlstr, "ftp://%99[^:]:%99[^@]@%99[^/]/%99[^\n]", info->user, info->pass, info->host, info->path);
	if (n < 4){
		printf("URL is incomplete...");
		
	}

}

int main(int argc, char **argv) {
	int socketfd;
	int socketfdClient = -1;
	struct sockaddr_in server_addr;
	struct sockaddr_in server_addr_client;
	urlStruct url_info;
	char responseCode[3];

	struct hostent *ip;

	infoStruct(argv[1], &url_info);

	printf(" - Username: %s\n", url_info.user);
	printf(" - Password: %s\n", url_info.pass);
	printf(" - Host: %s\n", url_info.host);
	printf(" - Path: %s\n", url_info.path);
	printf(" - Filename: %s\n", url_info.filename);

	ip = getip(url_info.host);

	printf(" - IP Address : %s\n\n", inet_ntoa(*((struct in_addr *)ip->h_addr)));

	/*server address handling*/
	bzero((char *)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;

	/*32 bit Internet address network byte ordered*/
	server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *)ip->h_addr)));
	
	/*server TCP port must be network byte ordered */
	server_addr.sin_port = htons(SERVER_PORT);

	/*open an TCP socket*/
	if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket()");
		exit(0);
	}
	
	/*connect to the server*/
	printf("Connecting to server...\n");
	if (connect(socketfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("connect()");
		exit(0);
	}

	printf("Reading response code from server...\n");
	readServerResponse(socketfd, responseCode); 
	printf("Response code: %s\n", responseCode);

	/* Positive Completion reply. The requested action has been successfully completed.
	A new request may be initiated. */
	if (responseCode[0] == '2') {
		printf("Success. Connection established.\n");
	}

	printf("Sending Username\n");
	int res = sendCommand(socketfd, "user ", url_info.user, url_info.filename, socketfdClient);

	if (res == 1) {
		printf("Sending Password\n");
		res = sendCommand(socketfd, "pass ", url_info.pass, url_info.filename, socketfdClient);
	}

	// "listen" on data port
	write(socketfd, "pasv\n", 5);
	int serverPort = getServerPort(socketfd);
	printf("serverPort: %d\n", serverPort);

}


//gets ip address according to the host's name
struct hostent *getip(char host[])
{
	struct hostent *h;

	if ((h = gethostbyname(host)) == NULL)
	{
		herror("gethostbyname");
		exit(1);
	}

	return h;
}

//reads response code from the server
void readServerResponse(int socketfd, char *responseCode) {
	int state = 0;
	int index = 0;
	char c;

	while (state != 3) {	
		read(socketfd, &c, 1);
		switch (state) {
		//waits for 3 digit number followed by ' ' or '-'
		case 0:
			if (c == ' ') {
				if (index != 3) {
					printf(" > Error receiving response code\n");
					return;
				}
				index = 0;
				state = 1;
			}
			else {
				if (c == '-') {
					state = 2;
					index=0;
				}
				else {
					if (isdigit(c)) {
						responseCode[index] = c;
						index++;
					}
				}
			}
			break;
		// reads until the end of the line
		case 1:
			if (c == '\n')
				state = 3;
			break;
		// waits for response code in multiple line responses
		case 2:
			if (c == responseCode[index])
				index++;
			else {
				if (index == 3 && c == ' ')
					state = 1;
				else 
				  if (index == 3 && c == '-')
					index = 0;
			}
			break;
		}
	}
}
int sendCommand(int socketfd, char cmd[], char commandContent[],
							char* filename, int socketfdClient) {
	char responseCode[3];
	int action = 0;

	// sends the command
	write(socketfd, cmd, strlen(cmd));
	write(socketfd, commandContent, strlen(commandContent));
	write(socketfd, "\n", 1);

	while (1) {
		// reads the response
		readServerResponse(socketfd, responseCode);
		action = responseCode[0] - '0';

		switch (action) {
			// expect another reply before proceeding with a new command
			case 1:
				if (strcmp(cmd, "retr ") == 0) {
					createFile(socketfdClient, filename);
					break;
				}
				readServerResponse(socketfd, responseCode);
				break;

			// command accepted, we can send another command
			case 2:
				return 0;
			// user should send another command; command group
			case 3:
				return 1;
			// try again
			case 4:
				write(socketfd, cmd, strlen(cmd));
				write(socketfd, commandContent, strlen(commandContent));
				write(socketfd, "\r\n", 2);
				break;
			case 5:
				printf("Command not accepted\n");
				close(socketfd);
				exit(-1);
			}
		}
}
int getServerPort(int socketfd) {
	char c;
	int index = 0, state = 0;
	char pos_5[MAX_PORT_LEN];
	char pos_6[MAX_PORT_LEN];

	while(state < 7) {
		read(socketfd, &c, 1);
		switch (state) {
			case 0:
				if (c == ' ') {
					if (index != 3) {
						printf("error\n");
						return -1;
					} else {
						index = 0;
						state = 1;
					}
				} else {
					index++;
				}
				break;

			case 5:
				if (c == ',') {
					state++;
					index = 0;
				} else
					pos_5[index++] = c;
				break;
			case 6:
				if (c == ')') { // finish
					state++;
				} else {
					pos_6[index++] = c;
				}
				break;
			default:
				if (c == ',')
					state++;
				break;
		}
	}
	int firstByteInt = atoi(pos_5);
	int secondByteInt = atoi(pos_6);
	return (firstByteInt * 256 + secondByteInt);
}

void createFile(int fd, char* filename) {
	FILE *file = fopen((char *)filename, "wb+");

	char bufSocket[SOCKET_BUF_SIZE];
 	int bytes;
 	while ((bytes = read(fd, bufSocket, SOCKET_BUF_SIZE))>0) {
    	bytes = fwrite(bufSocket, bytes, 1, file);
    }

  	fclose(file);

	printf(" > Finished downloading file\n");
}