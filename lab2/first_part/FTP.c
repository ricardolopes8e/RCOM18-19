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

#define SERVER_PORT 6000

typedef struct url {
	char * user;
	char * password;
	char * host;
	char * url_path;
} url;

void infoStruct(char * urlstr, url * info){
	

	char * temp = malloc(urlstr);
	strcpy(temp,urlstr);

	info->user = calloc(strlen(temp), sizeof(char));
	info->password = calloc(strlen(temp), sizeof(char));
	info->host = calloc(strlen(temp), sizeof(char));
	info->url_path = calloc(strlen(temp), sizeof(char));

	temp = strtok(temp+6, ":");
	strcpy(info->user, temp);
	temp = strtok(NULL, "@");
	strcpy(info->password, temp);
	temp = strtok(NULL, "/");
	strcpy(info->host, temp);
	temp = strtok(NULL, "");
	strcpy(info->url_path, temp);

}


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

void serverAnswer(int sockfd, char *response){
	int state = 0;
	int index = 0;
	char c;

	while (state != 3)
	{	
		read(sockfd, &c, 1);
		printf("%c", c);
		switch (state)
		{
		//waits for 3 digit number followed by ' ' or '-'
		case 0:
			if (c == ' ')
			{
				if (index != 3)
				{
					printf(" > Error receiving response code\n");
					return;
				}
				index = 0;
				state = 1;
			}
			else
			{
				if (c == '-')
				{
					state = 2;
					index=0;
				}
				else
				{
					if (isdigit(c))
					{
						response[index] = c;
						index++;
					}
				}
			}
			break;
		//reads until the end of the line
		case 1:
			if (c == '\n')
			{
				state = 3;
			}
			break;
		//waits for response code in multiple line responses
		case 2:
			if (c == response[index])
			{
				index++;
			}
			else
			{
				if (index == 3 && c == ' ')
				{
					state = 1;
				}
				else 
				{
				  if(index==3 && c=='-'){
					index=0;
					
				}
				}
				
			}
			break;
		}
}
}

int main(int argc, char** argv){

	if (argc < 2){
		printf("Usage:\tdownload ftp://[<user>:<password>@]<host>/<url-path>\n");
		exit(1);
	}

	url *URL_INFO = malloc(sizeof(url));

	infoStruct(argv[1], URL_INFO);

	int	sockfd;
	struct hostent *ip;
	char answer[3];
	struct sockaddr_in server_addr;


	printf(" - Username: %s\n", URL_INFO->user);
	printf(" - Password: %s\n", URL_INFO->password);
	printf(" - Host: %s\n", URL_INFO->host);
	printf(" - Path :%s\n", URL_INFO->url_path);

/*
	ip = getip(URL_INFO->host);

	printf(" - IP Address : %s\n\n", inet_ntoa(*((struct in_addr *)ip->h_addr)));

	//server address handling
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *)ip->h_addr)));	
	server_addr.sin_port = htons(SERVER_PORT);		
 
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    	perror("socket()");
        exit(0);
    }

	
    if(connect(sockfd,(struct sockaddr *)&server_addr,sizeof(server_addr)) < 0){
        	perror("connect()");
		exit(0);
	}     

	serverAnswer(sockfd, answer);

	if(answer[0] == '2'){
		printf("Connection Estabilished\n");
	}

	*/
	
	return 0;


	//clientTCP.c


}
