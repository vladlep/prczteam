#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include "netio.h"


#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 50678
#define BUFSIZE 1024

int main() {
  
	int sockfd;
	char buf[BUFSIZE],data[BUFSIZE-7];
	struct sockaddr_in local_addr,remote_addr;
	int nread;
	char b1,b2;
	
	printf("\nMaking a socket");
	sockfd = socket(AF_INET,SOCK_STREAM, 0);
	if(sockfd == -1)
	{
		printf("\nCould not make a socket\n");
	    return -1;
	}
	
	if(set_addr(&local_addr, NULL, INADDR_ANY, 0) == -1)
	{
		printf("\nCould not fill local address\n");
		return -1;
	}
	
	printf("\nBinding to port");
	if(bind(sockfd, (struct sockaddr *) &local_addr, sizeof(local_addr)) == -1)
	{
		printf("\nCould not bind to port\n");
	    return -1;
	}
	
	if(set_addr(&remote_addr, SERVER_ADDRESS, 0, SERVER_PORT) == -1)
	{
		printf("\nCould not fill remote address\n");
		return -1;
	}
	
	printf("\nConnecting to server");
	if(connect(sockfd, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) == -1) 
	{
	        printf("\nCould not connect to host\n");
	        return -1;
	}
	
	strcpy(data,"This is a message from a client");
	if( set_data(buf,data,2) < 0)
	{
		printf("\nERROR\n");
		return -1;
	}
	printf("\nSending message to server: %s", buf);
	stream_write(sockfd, buf, BUFSIZE);
	
	memset(buf,'\0',BUFSIZE);
	nread = stream_read(sockfd, buf, BUFSIZE);
	if(nread < 0)
		printf("\nRead error from server\n");
	else
		printf("\nReceived message from server: %s\n",buf);
	
	if(close(sockfd) == -1)
	{
		printf("\nCould not close socket\n");
		return -1;
	}
	return 0;
}
