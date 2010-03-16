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
#include <sys/wait.h>
#include <dirent.h>

#include "client.h"
#include "netio.h"

#define C_MSG_BEGIN "00"
#define C_MSG_END "11"
#define C_MSG_FILENAME "01"

#define S_MSG_FILEDIMENSION "10"
#define S_MSG_BUSY "01"

#define FILENAME_LENGTH 4

#define BUFSIZE 1024
#define TREE "tree.txt"


struct nod *first = NULL, *last = NULL;
char home_dir[1024];


int main(int argc, char *argv[])
{

	char server_address[255];
	int server_port;

	if( argc == 2 )
	{
		getcwd(home_dir, sizeof(home_dir));
		strcpy(server_address, "127.0.0.1");
		server_port = atoi(argv[1]);
	} else
	{
		if( argc == 3 )
		{
			strcpy(home_dir, argv[1]);
			strcpy(server_address, "127.0.0.1");
			server_port = atoi(argv[2]);
		} else
		{
			if( argc == 4 )
			{
				strcpy(home_dir, argv[1]);
				strcpy(server_address, argv[2]);
				server_port = atoi(argv[3]);
			} else
			{
				printf("\nSyntax: %s [dir] [server-address] server-port\n", argv[0]);
				return -1;
			}
		}
	}
	
	if( startConnection(server_address, server_port) == -1)
	{
		printf("\nAn error occured during connection\n");
		return -1;
	}

	return 0;
}

int startConnection(char *server_address, int server_port)
{
	int sockfd;
	struct sockaddr_in local_addr, remote_addr;

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
	
	if(set_addr(&remote_addr, server_address, 0, server_port) == -1)
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
	
	printf("\nConnected\n");

	if( chdir(home_dir) == -1)
	{
		printf("\nCould not change directory to %s\n", home_dir);
		return -1;
	}
	
	switch( readTree(sockfd) )
	{
		case 0: break;
		case -1: printf("\nAn error occurred while trying to receive the file-tree\n");
			return -1;
		case 1: printf("\nRetrying in 10 seconds\n");
			sleep(10);
			startConnection(server_address, server_port);
			break;
	}

	printf("\nStarting update\n");
	if( update(sockfd) == -1)
	{
		printf("\nAn error ocurred while updating\n");
		return -1;
	}

	if(close(sockfd) == -1)
	{
		printf("\nCould not close socket\n");
		return -1;
	}

	if ( removeOldFiles(home_dir) == -1)
	{
		printf("\nAn error occurred while removing old files\n");
		return -1;
	}
	printf("\nUpdate finished\n");

	return 0;
}

int readTree(int sockfd)
{

	char buf[BUFSIZE],type[3],len[3],dim[12];
	int nread;

	memset(buf, '\0', BUFSIZE);
	snprintf(buf, strlen(C_MSG_BEGIN) + 1, "%s", C_MSG_BEGIN);
	stream_write(sockfd, (void *)buf, strlen(C_MSG_BEGIN));

	memset(type, '\0', sizeof(type));
	nread = stream_read(sockfd, (void *)type, sizeof(type)-1);

	if( nread < 0 )
	{
		printf("\nRead error from socket\n");
		return -1;
	}

	type[sizeof(type)-1] = '\0';

	if( strcmp(type,S_MSG_FILEDIMENSION) == 0 )
	{
		memset(len, '\0', sizeof(len));
		nread = stream_read(sockfd, (void *)len, sizeof(len)-1);

		if( nread < 0 )
        	{
                	printf("\nRead error from socket\n");
                	return -1;
        	}

		len[sizeof(len)-1] = '\0';

		memset(dim, '\0', sizeof(dim));
		nread = stream_read(sockfd, (void *)dim, atoi(len));

		if( nread < 0 )
                {
                        printf("\nRead error from socket\n");
                        return -1;
                }

                dim[atoi(len)] = '\0';

		int size = atoi(dim);
		
		if( receiveFile(sockfd, TREE, size, O_CREAT | O_TRUNC | O_WRONLY) == -1)
		{
			printf("\nAn error occurred while receiving file %s\n", TREE);
			return -1;
		}

	} else
	{
		if( strcmp(type, S_MSG_BUSY) == 0)
		{
			printf("\nServer is busy\n");

			memset(buf, '\0', BUFSIZE);
        		snprintf(buf, strlen(C_MSG_END) + 1, "%s", C_MSG_END);
        		stream_write(sockfd, (void *)buf, strlen(C_MSG_END));

			return 1;
		} else
		{
			printf("\nUnrecognized message\n");
			return -1;
		}
	}
	
	return 0;
}

int update(int sockfd)
{
	char line[1024],name[1024],buf[1024],type = 'x';
	int size = -1, time = -1, nread;
	struct stat st_buf;

	FILE *f = fopen(TREE, "r");

	if(f == NULL)
        {
        	printf("\nCould not open file %s", TREE);
                return -1;
        }

	while( fgets(line, 1024, f) != NULL )
	{
		int i = 0;
		char *str[4];
        	char *aux = (char*)malloc(strlen(line)*sizeof(char));

        	strcpy(aux,line);
        	do
        	{
                	str[i++] = strsep(&aux,":");
                	
        	}while(aux != NULL);
		
		type = str[0][0];
		strcpy(name,str[1]);
		size = atoi(str[2]);
		time = atoi(str[3]);
		
		filelistAdd(name);

		if( stat(name,&st_buf) == -1)
		{
			if(errno == ENOENT)
			{
				if( type == 'd' )
				{
					if( mkdir(name, 0777) == -1)
					{
						printf("\nCould not make directory %s\n", name);
						return -1;
					}
					
					printf("\nDirectory %s added\n", name);
				} else
				{
					if( type == 'f' )
					{
						memset(buf, '\0', BUFSIZE);
						snprintf(buf, strlen(C_MSG_FILENAME) + 1, "%s", C_MSG_FILENAME);
						snprintf(&buf[strlen(C_MSG_FILENAME)], FILENAME_LENGTH + 1, "%.4d", strlen(name));
						snprintf(&buf[FILENAME_LENGTH + strlen(C_MSG_FILENAME)], strlen(name) + 1, "%s", name);
					
						stream_write(sockfd, (void *)buf, strlen(buf));
						
						if( receiveFile(sockfd, name, size, O_CREAT | O_TRUNC | O_WRONLY) == -1)
						{
							printf("\nAn error occurred while receiving file %s\n", name);
							return -1;
						}

						struct utimbuf ut_buf;
						ut_buf.modtime = time;
						if( utime(name, &ut_buf) == -1)
						{
							printf("\nCould not update last modified time for file %s\n", name);
							return -1;
						}
						
						printf("\nFile %s added\n", name);
					}
				} 
			}
		} else
		{
			if( (type == 'f') && ((st_buf.st_size != size) || (st_buf.st_mtime != time)) )
			{
				memset(buf, '\0', BUFSIZE);
                                snprintf(buf, strlen(C_MSG_FILENAME) + 1, "%s", C_MSG_FILENAME);
                                snprintf(&buf[strlen(C_MSG_FILENAME)], FILENAME_LENGTH + 1, "%.4d", strlen(name));
				snprintf(&buf[FILENAME_LENGTH + strlen(C_MSG_FILENAME)], strlen(name) + 1, "%s", name);
                                stream_write(sockfd, (void *)buf, strlen(buf));

                                if( receiveFile(sockfd, name, size, O_TRUNC | O_WRONLY) == -1)
				{
					printf("\nAn error occurred while receiving file %s\n", name);
					return -1;
				}

				struct utimbuf ut_buf;
                                ut_buf.modtime = time;
                                if( utime(name, &ut_buf) == -1)
				{
					printf("\nCould not update last modified time for file %s\n", name);
					return -1;
				}

				printf("\nFile %s updated\n", name);
			}
		}
	} 
	
	memset(buf, '\0', BUFSIZE);
	snprintf(buf, strlen(C_MSG_END) + 1, "%s", C_MSG_END);
	stream_write(sockfd, (void *)buf, strlen(C_MSG_END));

	return 0;
}

int receiveFile(int sockfd, char *name, int size, int mask)
{

	int nread = 1;
	int fd;
	char buf[BUFSIZE];

	if( (mask & O_CREAT) )
	{
		fd = open(name, mask, 0644);
	}
	else
	{
		fd = open(name, mask);
	}
        if(fd == -1)
        {
        	printf("\nCould not open file %s", name);
                return -1;
        }

	while( (size > 0) && (nread > 0) )
	{
               	nread = stream_read(sockfd, (void *) buf, size < BUFSIZE ? size : BUFSIZE);
                if(nread > 0)
                {
                       	size -= nread;
                        write(fd, (void *)buf, nread);
                }
        }

	if( nread < 0 )
        {
        	printf("\nRead error from socket\n");
                return -1;
        }

	if( size != 0 )
	{
		printf("\nSize error for file %s\n", name);
		return -1;
	}

	if(close(fd) < 0)
        {
        	printf("\nCould not close file %s\n", name);
                return -1;
        }

	return 0;
}

int removeOldFiles(char path[])
{
	int status;
	DIR *wdir;
	wdir = opendir(path);

	if(wdir == NULL)
	{
		printf("\nCould not open directory %s\n", path);
		return -1;
	}

	struct dirent *drnt;
	struct stat st_buf;
	
	char new_path[1024],name[1024];
	
	while( (drnt = readdir(wdir)) != NULL)
	{
		
		strcpy(new_path, path);
		strcat(new_path, "/");
		strcat(new_path, drnt->d_name);
		if( stat(new_path,&st_buf) == -1)
		{
			printf("\nStat error for %s\n", new_path);
			return -1;
		} else
		{
			strcpy(name, new_path + strlen(home_dir) + 1);
			if( S_ISDIR(st_buf.st_mode) && strcmp(drnt->d_name,".") && strcmp(drnt->d_name,"..") )
			{	
				if( filelistContains(name) )
				{	
					removeOldFiles(new_path);
				} else
				{
					int pidd;
					pidd = vfork();
					switch(pidd)
					{
					case -1: printf("\nFork Error\n");
						return -1;
					case 0: execlp("rm", "remove", "-r", new_path, (char*)0);
						exit(0);
					default: wait(&status);
						printf("\nRemoved directory %s\n", name);
					}
				}
			}
	
			if( S_ISREG(st_buf.st_mode) )
			{	
				if( !filelistContains(name) )
				{
					int pidf;
					pidf= vfork();
					switch(pidf)
					{
					case -1: printf("\nFork Error\n");
						return -1;
					case 0: execlp("rm", "remove", new_path, (char*)0);
						exit(0);
					default: wait(&status);
						if(strcmp(name, TREE))
							printf("\nRemoved file %s\n", name);
					}
				}
			}
		}
	}

	return 0;
}

void filelistAdd(char name[1024])
{
	struct nod *new = (struct nod*)malloc(sizeof(struct nod));
	strcpy(new->name, name);
	new->next = NULL;
	if(first == NULL)
	{
		first = new;
		last = new;
	} else
	{
		if(first == last)
		{
			last = new;
			first->next = last;
		} else
		{
			last->next = new;
			last = new;
		}
	}
}

int filelistContains(char *name)
{
	struct nod *p;
	for(p = first; p != NULL; p = p->next)
	{
		if(strcmp(p->name,name) == 0)
			return 1;
	}
	return 0;
}
