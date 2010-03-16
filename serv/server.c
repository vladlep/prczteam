#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "server.h"
#include "netio.h"

#define C_MSG_BEGIN "00"
#define C_MSG_END "01"
#define C_MSG_FILENAME "02"

#define S_MSG_FILEDIMENSION "03"
#define S_MSG_BUSY "04"

#define DIMENSION_LENGTH 2
#define FILENAME_LENGTH 4

#define SHMKEY 0x100
#define MAX_QUEUE 5
#define BUFSIZE 1024
#define TREE "tree.txt"


int *p_busy;
int *c_busy;
char home_dir[1024];
int sockfd;
struct sockaddr_in rmt_addr;
socklen_t rlen;


int main(int argc, char *argv[])
{

	struct sockaddr_in local_addr;
	int server_port;
	
	if( argc == 3)
	{
		strcpy(home_dir,argv[1]);
		server_port = atoi(argv[2]);
	} else
	{
		printf("\nSyntax: %s dir port\n", argv[0]);
		return -1;
	}
	
	int shmid;
	key_t key;
	int *shm;
	key = (key_t) SHMKEY;

	if( (shmid = shmget(key, sizeof(int), IPC_CREAT | 0666)) < 0)
	{
        	printf("\nAn error occurred while creating the shared memory segment\n");
        	return -1;
    	}

	if( (shm = shmat(shmid, NULL, 0)) == (int *) -1)
	{
        	printf("\nAn error occurred while attaching the shared memory segment\n");
        	return -1;
	}
	p_busy = shm;
	*p_busy = 0;	

	if( writeTree() == -1)
	{
		printf("\nAn error occurred while writing file-tree\n");
		return -1;
	}

	if( startServer(server_port) == -1 )
	{
		printf("\nAn error occurred while starting server\n");
		return -1;
	}

	acceptConnections();

	exit(0);
}

int startServer(int server_port)
{
	struct sockaddr_in local_addr;

	printf("\nMaking socket");
	sockfd = socket(AF_INET,SOCK_STREAM, 0);
	if(sockfd == -1)
	{
	        printf("\nCould not make a socket\n");
	        return -1;
	}

	if(set_addr(&local_addr, NULL, INADDR_ANY, server_port) == -1)
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
	
	if(listen(sockfd, MAX_QUEUE) == -1)
	{
		printf("\nCould not listen\n");
	    	return -1;
	}
	
	rlen = sizeof(rmt_addr);

	return 0;
}

int writeTree()
{
	int fd;
	struct stat st_buf;
	
	if(stat(home_dir,&st_buf) == -1)
	{
		printf("\nStat error for %s\n", home_dir);		
		return -1;
	}

	if(!S_ISDIR(st_buf.st_mode))		
	{
		printf("\n%s is not a directory\n", home_dir);
		return -1;
	}
	
    	if( (fd = open(TREE, O_CREAT | O_TRUNC | O_WRONLY, 0644)) == -1 )
	{
		printf("\nCould not open file %s", TREE);
       		return -1;
   	}
    
	createTree(home_dir, fd);
	
	if( close(fd)  < 0 )
    	{
       		printf("\nCould not close file %s\n", TREE);
       		return -1;
    	}

	return 0;
}

void createTree(char path[], int fd)
{
	DIR *wdir;
	wdir = opendir(path);
	
	if(wdir == NULL)
	{
		printf("\nCould not open directory %s\n", path);
		exit(1);
	}

	struct dirent *drnt;
	struct stat st_buf;
	
	char new_path[1024],line[1024],aux[11];

	while( (drnt = readdir(wdir)) != NULL)
	{
		strcpy(new_path, path);
		strcat(new_path, "/");
		strcat(new_path, drnt->d_name);

		if( stat(new_path,&st_buf) == -1)
		{
			printf("\nStat error for %s\n", new_path);
			if(close(fd) == -1)
			{
				printf("\nCould not close file %s\n", TREE);
				if( closedir(wdir) == -1 )
				{
					printf("\nError closing directory %s", path + strlen(home_dir) + 1);
					exit(1);
				}
				exit(1);
			}
			if( closedir(wdir) == -1 )
			{
				printf("\nError closing directory %s", path + strlen(home_dir) + 1);
				if(close(fd) == -1)
				{
					printf("\nCould not close file %s\n", TREE);
					exit(1);
				}
				exit(1);
			}
			exit(1);
		} else
		{
			if( strcmp(drnt->d_name,".") && strcmp(drnt->d_name,"..") )
			{
				if( S_ISDIR(st_buf.st_mode) )
				{
					strcpy(line, "d:");
				}

				if( S_ISREG(st_buf.st_mode) )
				{	
					strcpy(line, "f:");
				}

				strcat(line, new_path + strlen(home_dir) + 1);
				strcat(line, ":");
				snprintf(aux, 11, "%d", (int)st_buf.st_size);
				strcat(line, aux);
				strcat(line, ":");
				snprintf(aux, 11, "%d", (int)st_buf.st_mtime);
				strcat(line, aux);
				strcat(line,"\n");

				if( write(fd, (void *)line, strlen(line)) == -1)
				{
					printf("\nError writing to file %s\n", TREE);
					if(close(fd) == -1)
					{
						printf("\nCould not close file %s\n", TREE);
						if( closedir(wdir) == -1 )
						{
							printf("\nError closing directory %s", path + strlen(home_dir) + 1);
							exit(1);
						}
						exit(1);
					}	
					if( closedir(wdir) == -1 )
					{
						printf("\nError closing directory %s", path + strlen(home_dir) + 1);
						if(close(fd) == -1)
						{
							printf("\nCould not close file %s\n", TREE);
							exit(1);
						}
						exit(1);
					}
					exit(1);
				}

				if( S_ISDIR(st_buf.st_mode) )
				{	
					createTree(new_path, fd);
				}
			}
		}
	}

	if( closedir(wdir) == -1 )
	{
		printf("\nError closing directory %s", path + strlen(home_dir) + 1);
		if(close(fd) == -1)
		{
			printf("\nCould not close file %s\n", TREE);
			exit(1);
		}
		exit(1);
	}
}

void acceptConnections()
{

	int connfd;
	int pid;
	struct sigaction act;

	printf("\nServer ONLINE\n");

	sigemptyset(&act.sa_mask);
	act.sa_handler = updateServerTree;
	act.sa_flags = 0;
	act.sa_flags |= SA_NODEFER;
	sigaction(SIGINT, &act, NULL);
	
	while( 1 )
	{
		connfd = accept(sockfd, (struct sockaddr*)&rmt_addr, &rlen);
		
		pid = fork();

		switch(pid)
		{
		case -1: printf("\nFork Error\n");
			exit(1);
		case 0: sigemptyset(&act.sa_mask);
			act.sa_handler = SIG_IGN;
			act.sa_flags = 0;
			sigaction(SIGINT, &act, NULL);

			if( close(sockfd) == -1)
			{
				printf("\nCould not close socket %d\n", sockfd);
				exit(1);
			}
			
			int shmid;
			key_t key;
			int *shm;
			key = (key_t) SHMKEY;

			if( (shmid = shmget(key, sizeof(int), 0666)) < 0)
			{
        			printf("\nAn error occurred while locating the shared memory segment\n");
        			exit(1);
			}

			if( (shm = shmat(shmid, NULL, 0)) == (int *) -1)
			{
        			printf("\nAn error occurred while attaching the shared memory segment\n");
        			exit(1);
			}
			c_busy = shm;
			
			acceptMessages(connfd);

			exit(0);
		default: if( close(connfd) == -1)
			{
				printf("\nCould not close socket %d\n", connfd);
				exit(1);
			}
		}
	}
}

void acceptMessages(int connfd)
{

	int sw = 1, nread;
	char buf[BUFSIZE],name[1024],type[3],dim[12],len[3];

	while(sw)
	{
		memset(type, '\0', sizeof(type));
		nread = stream_read(connfd, type, sizeof(type) - 1);
		if( nread < 0 )
		{
			printf("\nRead error from socket\n");
			exit(1);
		}
		type[2] = '\0';
		
		if( strcmp(type, C_MSG_END) == 0 )
		{
			sw = 0;
		} else
		{
			if( strcmp(type, C_MSG_BEGIN) == 0)
			{
				if( *c_busy == 0 )
				{
					struct stat st_buf;

					if( stat(TREE, &st_buf) == -1)
					{
						printf("\nStat error for %s\n", TREE);
						exit(1);
					}

					snprintf(dim, strlen(S_MSG_FILEDIMENSION) + 1, "%s", S_MSG_FILEDIMENSION);
					int x = (int)log10((double)st_buf.st_size) + 1;
					snprintf(&dim[strlen(S_MSG_FILEDIMENSION)], DIMENSION_LENGTH + 1, "%.2d", x);
                                	snprintf(&dim[strlen(S_MSG_FILEDIMENSION) + DIMENSION_LENGTH], x+1, "%d", st_buf.st_size);
				
                                	stream_write(connfd, (void *)dim, strlen(dim));

					if( sendFile(connfd, TREE, O_RDONLY) == -1)
					{
						printf("\nAn error occurred while sending file %s\n", TREE);
						exit(1);
					}
				}
				else
				{
					memset(buf, '\0', BUFSIZE);
					snprintf(buf, strlen(S_MSG_BUSY) + 1, "%s", S_MSG_BUSY);
					stream_write(connfd, (void *)buf, strlen(S_MSG_BUSY));
				}
			} else
			{
				if( strcmp(type, C_MSG_FILENAME) == 0 )
				{
					chdir(home_dir);
					
					memset(len, '\0', FILENAME_LENGTH + 1);
					nread = stream_read(connfd, len, FILENAME_LENGTH);
					if( nread < 0 )
	                                {
                        	        	printf("\nRead error from socket\n");
        	                                exit(1);
                	                }
					len[FILENAME_LENGTH] = '\0';

					memset(name, '\0', 1024);
					nread = stream_read(connfd, name, atoi(len));
					if( nread < 0 )
	                                {
        	                        	printf("\nRead error from socket\n");
                	                        exit(1);
                        	        }
					name[atoi(len)] = '\0';

					if( sendFile(connfd, name, O_RDONLY) == -1)
					{
						printf("\nAn error occurred while sending file %s\n", name);
						exit(1);
					}	
				} else
				{
					printf("\nUnrecognized message\n");
					exit(1);
				}
			}
		}
	}
}

int sendFile(int connfd, char *name, int mask)
{

	int fd, nread;
	char buf[BUFSIZE];

	fd = open(name, mask);

	if(fd == -1)
	{
		printf("\nCould not open file %s", name);
		return -1;
	}
	
	while( 0 < (nread = read(fd, (void *)buf, BUFSIZE)))
	{
		stream_write(connfd, (void *)buf, nread);
	}

	if( nread < 0 )
	{
        	printf("\nRead error from file %s\n", name);
		if(close(fd) < 0)
		{
			printf("\nCould not close file %s\n", name);
			return -1;
		}
                return -1;
       	}

	if(close(fd) < 0)
	{
		printf("\nCould not close file %s\n", name);
		return -1;
	}
}

static void updateServerTree(int sig)
{

	pid_t done;
	int status;

	printf("\nWaiting for clients to end connections\n");

	*p_busy = 1;

	while(1)
	{
		done = wait(&status);
		if( done == -1)
		{
			if( errno == ECHILD )
				break;
		} else
		{
			if( !WIFEXITED(status) || (WEXITSTATUS(status) != 0) )
			{
				printf("\nProcess with pid %d failed\n", done);
			}
		}
	}

	printf("\nAll connections ended\n");

	if( writeTree() == -1)
	{
		printf("\nAn error occurred while writing file-tree\n");
		exit(1);
	}

	printf("\nUpdate complete\n");

	*p_busy = 0;

	acceptConnections();
}
