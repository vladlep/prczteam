#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include "netio.h"

#define SERVER_PORT 50678
#define BUFSIZE 1024

int main()
{

	int sockfd, connfd;
	struct sockaddr_in local_addr, rmt_addr;
	socklen_t rlen;
	char buf[BUFSIZE];
	int pid,nread;
	
	printf("\nMaking socket");
	sockfd = socket(AF_INET,SOCK_STREAM, 0);
	if(sockfd == -1)
	{
	        printf("\nCould not make a socket\n");
	        return -1;
	}

	if(set_addr(&local_addr, NULL, INADDR_ANY, SERVER_PORT) == -1)
	{
		
		printf("\nCould not fill address\n");
		return -1;
	}
	
	printf("\nBinding to port");
	if( bind(sockfd, (struct sockaddr *) &local_addr, sizeof(local_addr)) == -1 )
	{
		printf("\nCould not bind to port\n");
	    return -1;
	}
	
	if(listen(sockfd,5) == -1)
	{
		printf("\nCould not listen\n");
	    return -1;
	}
	
	rlen = sizeof(rmt_addr);
	
	while(1)
	{
		connfd = accept(sockfd, (struct sockaddr*)&rmt_addr, &rlen);
		
		pid = fork();
		switch(pid)
		{
		case -1: printf("\nFork Error\n");
				return -1;
		case 0: close(sockfd);
				nread = read(connfd, buf, BUFSIZE);
				if(nread < 0)
				{
					printf("\nRead error from socket %d\n", connfd);
					memset(buf,'\0',BUFSIZE);
					strcpy(buf,"NACK");
				}
				else
				{
					printf("\nReceived message from %d: %s", connfd, buf);
					memset(buf,'\0',BUFSIZE);
					strcpy(buf,"ACK");
				}
				printf("\nSending message to %d: %s\n", connfd, buf);
				stream_write(connfd, buf, strlen(buf));
				exit(0);
		default: close(connfd);
		}
	}
	exit(0);
}
