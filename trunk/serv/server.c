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

#include <sys/stat.h>
#include <dirent.h>


#include "netio.h"

#define SERVER_PORT 50678
#define BUFSIZE 1024
#define TREE "tree.txt"
#define HOMEDIR "/home/marius/Desktop/test/"

char homeDir[255];

void write_tree(char path[],int f_desc);


int main(int argc, char *argv[])
{

	int sockfd, connfd;
	struct sockaddr_in local_addr, rmt_addr;
	socklen_t rlen;
	char buf[BUFSIZE],data[1022];
	char type[3],dim[12],len[5],name[1024];
	int pid,nread,status;
	
	int f_desc;
	struct stat st;
//initializing tree
// error checking
	if(argc!=2)
	 {
	  printf("No path introduced");
	  return -1;	
	 }	

	if(stat(argv[1],&st)!=0)
	{
		printf("Nu ati introdus o cale corecta\n");		
		return -1;
	}

	if(!S_ISDIR(st.st_mode))		
	{
		printf("Nu ati introdus o cale de director\n");
		return -1;
	}
	
	strcpy(homeDir,argv[1]);
	
    if((f_desc=open("tree.txt",O_CREAT | O_TRUNC | O_WRONLY, 0644))==-1)
	{
		printf("\nCould not open file %s", argv[1]);
       	return -1;
   	}
//end of error checking
    
	write_tree(argv[1],f_desc);
	
	
	if(close(f_desc) < 0)
    {
       	printf("\nCould not close file %s\n", argv[1]);
       	return -1;
    }   			


// end of initializing tree
	
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
			int sw = 1;
			chdir(HOMEDIR);
			while(sw)
			{
				memset(type,'\0',3);
				nread = stream_read(connfd, type, 2);
				if( nread < 0 )
				{
					printf("\nRead error from socket\n");
					exit(1);
				}
				type[2] = '\0';
				//printf("\n%s\n",type);
				if( strcmp(type,"11") == 0 )
				{
					sw = 0;
				} else
				{
					if( strcmp(type,"00") == 0 )
					{
						int fd;
						struct stat st_buf;
						fd = open(TREE,O_RDONLY);

						if(fd == -1)
						{
							printf("\n1Could not open file %s", TREE);
							exit(2);
						}

						fstat(fd, &st_buf);
						snprintf(dim, 3, "%c%c", '1', '0');
						int x = (int)log10((double)st_buf.st_size) + 1;
						snprintf(&dim[2], 3, "%.2d", x);
                                                snprintf(&dim[4], x+1, "%d",(int) st_buf.st_size);
						//printf("\n%s\n",dim);
                                                stream_write(connfd, (void *)dim, strlen(dim));

						while( 0 < (nread = read(fd, (void *)buf, 1024)))
						{
							//printf("\n%d - %s\n",nread,buf);
							stream_write(connfd, (void *)buf, nread);
						}
						if( nread < 0 )
		                                {
                		                        printf("\nRead error from file %s\n", TREE);
                                		        exit(3);
                                		}
						if(close(fd) < 0)
						{
							printf("\nCould not close file %s\n", TREE);
							exit(2);
						}

					} else
					{
						if( strcmp(type,"01") == 0 )
						{
							memset(len, '\0', 5);
							nread = stream_read(connfd, len, 4);
							if( nread < 0 )
	                                                {
                        	                                printf("\nRead error from socket\n");
        	                                                exit(1);
                	                                }
							len[4] = '\0';
							memset(name, '\0', 1024);
							nread = stream_read(connfd, name, atoi(len));
							if( nread < 0 )
	                                                {
        	                                                printf("\nRead error from socket\n");
                	                                        exit(1);
                        	                        }
							name[atoi(len)] = '\0';
							//printf("\n%s\n",name);	
							int fd;
							fd = open(name,O_RDONLY);

	                                                if(fd == -1)
        	                                        {
                	                                        printf("\nCould not open file %s", name);
                        	                                exit(2);
                                	                }
							while( 0 < (nread = read(fd, (void *)buf, 1024)))
                                                	{
                                                        	stream_write(connfd, (void *)buf, nread);
                                                	}
                                                	if( nread < 0 )
                                                	{
                                                        	printf("\nRead error from file %s\n", name);
                                                        	exit(3);
                                                	}
							if(close(fd) < 0)
							{
								printf("\nCould not close file %s\n", name);
                                                        	exit(2);
							}


						} else
						{
							printf("\nUnrecognized message\n");
							exit(4);
						}
					}
				}
			}
			
			exit(0);
		default: wait(&status); 
			close(connfd);
		}
	}
	exit(0);
}


void write_tree(char path[],int f_desc)
{
	DIR *d;
	d=opendir(path);
	struct dirent *aux;
	struct stat buf;
	
	char sir[100],sir_aux[120],aux2[25];
	int nr_pasi=0;

	while((aux=readdir(d)))
	{
		
		strcpy(sir,path);
		strcat(sir,"/");
		strcat(sir,aux->d_name);
		stat(sir,&buf);
		if(S_ISDIR(buf.st_mode) && strcmp(aux->d_name,".") && strcmp(aux->d_name,".."))
		{
			
			strcpy(sir_aux,"d:");						//identificator de director
			strcat(sir_aux,sir+strlen(homeDir)+1);  	// facem calea relativa
			strcat(sir_aux,":");
			snprintf(aux2,11,"%d",(int)buf.st_size);	// contine max 10 cifre numarul
			strcat(sir_aux,aux2);
			strcat(sir_aux,":");
			snprintf(aux2,11,"%d",(int)buf.st_mtime); 	// contine max 10 cifre numarul
			strcat(sir_aux,aux2);
			strcat(sir_aux,"\n");
			write(f_desc,(void *)sir_aux,strlen(sir_aux));
			write_tree(sir,f_desc);
		}
	
	if(S_ISREG(buf.st_mode))
		{	
			strcpy(sir_aux,"f:");					     //identificator de fisier
			strcat(sir_aux,sir+strlen(homeDir)+1); 		 // facem calea relativa
			strcat(sir_aux,":");
			snprintf(aux2,11,"%d",(int)buf.st_size);	 //contine max 10 cifre numarul
			strcat(sir_aux,aux2);
			strcat(sir_aux,":");
			snprintf(aux2,11,"%d",(int)buf.st_mtime);	 //contine max 10 cifre numarul
			strcat(sir_aux,aux2);
			strcat(sir_aux,"\n");


			write(f_desc,(void *)sir_aux,strlen(sir_aux));

		}
	
	}

}

