#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

#include "netio.h"

int set_addr(struct sockaddr_in * addr, char *name, u_int32_t inaddr, short sin_port) {
  
	struct addrinfo hints;
	struct addrinfo *res;
  	
	memset((void *)addr,0,sizeof(*addr));
	addr->sin_family = AF_INET;
	if(name != NULL) {
		
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_CANONNAME;
		
		if(getaddrinfo(name, NULL, &hints, &res))
			return -1;
		if(res == NULL)
			return -1;
		addr->sin_addr.s_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr.s_addr;
		freeaddrinfo(res);
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
