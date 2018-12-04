/*
 * Protocoale de comunicatii
 * Laborator 7 - TCP
 * Echo Server
 * client.c
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#define BUFLEN 1500

int main(int argc, char *argv[])
{
    int sockfd = 0;
    char buf[BUFLEN];
    struct sockaddr_in serv_addr; 

    if(argc != 3)
    {
        printf("\n Usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }

    // deschidere socket
    sockfd = socket(PF_INET, SOCK_STREAM, 0);

    // completare informatii despre adresa serverului
    serv_addr.sin_family = PF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_aton("127.0.0.1", &(serv_addr.sin_addr));

    // conectare la server
    connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
    
    // citire de la tastatura, trimitere de cereri catre server, asteptare raspuns
    
    while (1) {
        
        memset(buf, 0, BUFLEN);
        scanf("%s", buf);
        
        int nr = send(sockfd, buf, strlen(buf), 0);
        
        printf("[CLIENT] Trimis ");
        for (int i = 0; i < nr; i++)
                printf("%c", buf[i]);
            printf("\n");
    
        nr = recv(sockfd, buf, BUFLEN, 0);
        
        printf("[CLIENT] Primit ");

        for (int i = 0; i < nr; i++)
            printf("%c", buf[i]);
        printf("\n");

        if (strncmp(buf, "QUIT", 4) == 0)
            break;
    }

    memset(buf, 0, BUFLEN);
    recv(sockfd, buf, BUFLEN, 0);

    printf("[CLIENT] Primit %s\n", buf);
    // inchidere socket
    close(sockfd);

    return 0;
}
