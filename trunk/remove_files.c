#include <stdio.h>
#include <stdlib.h>
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
#include <dirent.h>
#include <malloc.h>

char homeDir[255];

void rec(char path[],FILE *f_desc);

int main(int argc, char *argv[])
{
	FILE *f_desc;
	struct stat buf;

	char path_dir_syn[255];

// error checking
	if(argc!=2)
	 {
	  printf("No path introduced");
	  return -1;	
	 }	

	
	if(stat(argv[1],&buf)!=0)
	{
		printf("Nu ati introdus o cale corecta\n");		
		return -1;
	}


	if(!S_ISDIR(buf.st_mode))		
	{
		printf("Nu ati introdus o cale de director\n");
		return -1;
	}
	
	strcpy(homeDir,argv[1]);
	
    if((f_desc=fopen("tree.txt","r"))==NULL)
	{
		printf("\nCould not open file tree.txt", argv[1]);
       	return -1;
   	}
	

    
	rec(argv[1],f_desc);
	
	
fclose(f_desc);
    

	return 0;
}

void rec(char path[],FILE *f_desc)
{
	DIR *d;
	d=opendir(path);
	struct dirent *aux;
	struct stat buf;
	
	char sir[1000],sir_aux[1200],aux2[1250];
	char *p;
	char sep[]={':'};

	fgets(sir_aux,1200,f_desc);
	
	while((aux=readdir(d)))
	{
		
		
		strcpy(sir,path);
		strcat(sir,"/");
		strcat(sir,aux->d_name);
		stat(sir,&buf);
		if(S_ISDIR(buf.st_mode) && strcmp(aux->d_name,".") && strcmp(aux->d_name,".."))
		{
				strncpy(aux2,sir_aux+2,strlen(sir)-strlen(homeDir)-1); 
				
			if(!strncmp(aux2,sir+strlen(homeDir)+1,strlen(sir)-strlen(homeDir)-1))
				rec(sir,f_desc);
			else
				{
				/*	printf("va fi sters %s \n\n",sir);
					puts(aux2);
					puts(sir+strlen(homeDir)+1);
					printf("%d",strlen(sir)-strlen(sir)-1);
				*/
				
					int pid_sterg_dir;
					if((pid_sterg_dir=vfork())<0)
					{
						printf("eroare la crearea unui proces");
						exit(-1);
					}
					if(pid_sterg_dir==0)
						execlp("rm","remove","-r",sir,(char*)0);
				
						//rmdir(sir); //sterg to directorul -r= recursiv
				}
		}
	
		if(S_ISREG(buf.st_mode))
		{	
					

			strncpy(aux2,sir_aux+2,strlen(sir)-strlen(homeDir)-1); 		 
		//	printf("\n\n");
			//puts(aux2);
			//printf("\n\n");

		
//			puts(sir_aux+2);
						
		/*	p=strtok(sir_aux,sep);
			printf("dupa primu strtok : %s\n",p);
			p=strtok(NULL,sep);
			printf("dupa al 2-lea strtok : %s\n",p);
			*/
	//		printf("     %s\n\n\n",(sir+strlen(homeDir)+1));			
//			printf("  nr=%d  ",strlen(sir)-strlen(homeDir)-1);
			
			if(!strncmp(aux2,sir+strlen(homeDir)+1,strlen(sir)-strlen(homeDir)-1))
			{	
//				printf("apare in ambele locuri: %s\n",sir+strlen(homeDir)+1);
				fgets(sir_aux,1200,f_desc);
			}
			else
			{
				printf("fisier de sters: %s",sir+strlen(homeDir)+1);	 //sterg fisierul 
				int pid_sterg_file;
				if((pid_sterg_file=vfork())<0)
					{
					printf("eroare la crearea unui proces");
					exit(-1);
					}
				if(pid_sterg_file==0)
					execlp("rm","remove",sir,(char*)0);
		
			}
		}
	
	}


}
