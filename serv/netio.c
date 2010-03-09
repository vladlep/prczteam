#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

#include "netio.h"

#define BUFSIZE 1024

int set_addr(struct sockaddr_in * addr, char *name, u_int32_t inaddr, short sin_port) {
  
	struct hostent *h;
  
	memset((void *)addr,0,sizeof(*addr));
	addr->sin_family = AF_INET;
	if(name != NULL) {
		h = gethostbyname(name);
		if(h == NULL)
			return -1;
		addr->sin_addr.s_addr = *(u_int32_t *)h->h_addr_list[0];
	} else
		addr->sin_addr.s_addr = htonl(inaddr);
	addr->sin_port = htons(sin_port);
	return 0;
}

int stream_read(int sockfd,void *buf,int len) {
  
	int nread;
	int remaining = len;

	while(remaining > 0) {
		if(-1 == (nread = read(sockfd, buf, remaining)))
			return -1;
		if(nread == 0)
			break;
		remaining -= nread;
		buf += nread;
	}
	return len - remaining;
}

int stream_write(int sockfd,void *buf,int len) {
  
	int nwr;
	int remaining = len;

	while(remaining > 0) {
		if(-1 == (nwr = write(sockfd, buf, remaining)))
			return -1;
		remaining -= nwr;
		buf += nwr;
	}
	return len - remaining;
}

int set_data(char buf[1024], char data[1017], int flag)
{

	int i;
	memset(buf,'\0',BUFSIZE);
	switch(flag)
	{	
		//ACK
		case 0: for(i = 0; i < BUFSIZE - 1; i++)
                                sprintf(&buf[i], "%c", '0');
			break;
		//NACK
		case 1: sprintf(buf, "%c%c", '0', '0');
			for(i = 2; i < BUFSIZE - 1; i++)
                                sprintf(&buf[i], "%c", '1');
			break;
		//MESSAGE
		case 2: sprintf(buf, "%c%c", '0', '1');
                        sprintf(&buf[2], "%.4d", strlen(data));
                        sprintf(&buf[6], "%s", data);
                        for(i = strlen(buf); i < BUFSIZE - 1; i++)
                                sprintf(&buf[i], "%c", '0');
                        break;
		//DATA
		case 3: sprintf(buf, "%c%c", '1', '0');
			sprintf(&buf[2], "%.4d", strlen(data));
			sprintf(&buf[6], "%s", data);
			for(i = strlen(buf); i < BUFSIZE - 1; i++)
				sprintf(&buf[i], "%c", '0');
			break;
		default: return -1;
	}
	return flag;
}
