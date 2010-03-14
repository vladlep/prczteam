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
#include <utime.h>
#include <errno.h>
#include "netio.h"


#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 50678
#define BUFSIZE 1024
#define TREE "tree.txt"

int update(int sockfd)
{
	char line[1024],name[1024],buf[1024];
	int size,time,nread;
	struct stat st_buf;
	FILE *f = fopen(TREE, "r");
	if(f == NULL)
        {
        	printf("\nCould not open file %s", TREE);
                return -1;
        }

	while(fgets(line,1024,f)!=NULL)
	{
		sscanf(line, "%s %d %d", name, &size, &time);
		if( stat(name,&st_buf) == -1)
		{
			if(errno == ENOENT)
			{
				if(name[strlen(name)-1] == '/')
				{
					mkdir(name, 0777);
				} else
				{
					memset(buf, '\0', 1024);
					snprintf(buf, 3, "%c%c", '0', '1');
					snprintf(&buf[2], 5, "%.4d", strlen(name));
					snprintf(&buf[6], strlen(name) + 1, "%s", name);
					//printf("\n%s\n",buf);
					stream_write(sockfd, (void *)buf, strlen(buf));
	
					int fd = open(name,O_CREAT | O_TRUNC | O_WRONLY, 0644);
                			if(fd == -1)
                			{
                        			printf("\nCould not open file %s", name);
                        			return -1;
                			}

                			while(size > 0)
                			{
                        			nread = stream_read(sockfd, (void *) buf, size < 1024 ? size : 1024);
                        			if(nread > 0)
                        			{
                                			size -= nread;
                                			write(fd, (void *)buf, nread);
                        			}
                			}
                			if(close(fd) < 0)
                			{
                        			printf("\nCould not close file %s\n", name);
                        			return -1;
                			}
					struct utimbuf ut_buf;
					ut_buf.modtime = time;
					utime(name, &ut_buf);

				}
			}
		} else
		{
			if( (name[strlen(name)-1] != '/') && ((st_buf.st_size != size) || (st_buf.st_mtime != time)) )
			{
				memset(buf, '\0', 1024);
                                snprintf(buf, 3, "%c%c", '0', '1');
                                snprintf(&buf[2], 5, "%.4d", strlen(name));
                                snprintf(&buf[6], strlen(name) + 1, "%s", name);
                                stream_write(sockfd, (void *)buf, strlen(buf));

                                int fd = open(name, O_TRUNC | O_WRONLY);
                                if(fd == -1)
                                {
                                	printf("\nCould not open file %s", name);
                                        return -1;
                                }

                                while(size > 0)
                                {
                                	nread = stream_read(sockfd, (void *) buf, size < 1024 ? size : 1024);
                                        if(nread > 0)
                                        {
                                        	size -= nread;
                                                write(fd, (void *)buf, nread);
                                        }
                                }
                                if(close(fd) < 0)
                                {
                                	printf("\nCould not close file %s\n", name);
                                        return -1;
                                }
				struct utimbuf ut_buf;
                                ut_buf.modtime = time;
                                utime(name, &ut_buf);
			}
		}
	} 
	memset(buf, '\0', 1024);
	snprintf(buf, 3, "%c%c", '1','1');
	stream_write(sockfd, (void *)buf, 2);
	return 0;

}

void remove_old_files()
{
}

int main() {
  
	int sockfd;
	char buf[BUFSIZE];
	char type[3],len[5],dim[12];
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
	
	memset(buf, '\0', 1024);
	snprintf(buf, 3, "%c%c", '0', '0');
	stream_write(sockfd, (void *)buf, 2);
	
	memset(type, '\0', 3);
	nread = stream_read(sockfd, (void *)type, 2);
	if( nread < 0 )
	{
		printf("\nRead error from socket\n");
		return -1;
	}
	type[2] = '\0';
	if( strcmp(type,"10") == 0 )
	{
		//printf("\nxxx\n");
		memset(len, '\0', 5);
		nread = stream_read(sockfd, (void *)len, 2);
		if( nread < 0 )
        	{
                	printf("\nRead error from socket\n");
                	return -1;
        	}
		len[2] = '\0';
		//printf("\n%s\n",len);
		memset(dim, '\0', 12);
		nread = stream_read(sockfd, (void *)dim, atoi(len));

		if( nread < 0 )
                {
                        printf("\nRead error from socket\n");
                        return -1;
                }
                dim[atoi(len)] = '\0';
		//printf("\n%s\n",dim);
		int fd = open(TREE, O_CREAT | O_TRUNC | O_WRONLY, 0644);
		if(fd == -1)
                {
                	printf("\nCould not open file %s", TREE);
             		return -1;
                }

		int size = atoi(dim);
		//printf("\n%d\n",size);
        	while(size > 0)
        	{
                	nread = stream_read(sockfd, (void *) buf, size < 1024 ? size : 1024);
			//printf("\n%d\n",nread);
                	if(nread > 0)
                	{
                        	size -= nread;
                        	write(fd, (void *)buf, nread);
                	}
        	}
		if(close(fd) < 0)
		{
			printf("\nCould not close file %s\n", TREE);
			return -1;
		}


	}

	update(sockfd);

	if(close(sockfd) == -1)
	{
		printf("\nCould not close socket\n");
		return -1;
	}
	return 0;

	remove_old_files();
}
