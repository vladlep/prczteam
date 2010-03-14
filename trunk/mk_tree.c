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

void rec(char path[],int f_desc);

int main(int argc, char *argv[])
{
	int f_desc;
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
	
    if((f_desc=open("tree.txt",O_CREAT | O_TRUNC | O_WRONLY, 0644))==-1)
	{
		printf("\nCould not open file %s", argv[1]);
       	return -1;
   	}
	
/*	
	getcwd(path_dir_syn,200);
	strcat(path_dir_syn,"/");
	strcat(path_dir_syn,argv[1]);
	strcat(path_dir_syn,"/");
	printf("   %s  ",path_dir_syn);
	
//		chdir(path_dir_syn); --nu merge , doar daca il pun inainte de a apela stat // schimbam dir curent, pt a obtine cai relative si nu absolute
*/

//end of error checking
    
	rec(argv[1],f_desc);
	
	
	if(close(f_desc) < 0)
    {
       	printf("\nCould not close file %s\n", argv[1]);
       	return -1;
    }   			
    

	return 0;
}

void rec( char path[],int f_desc)
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
			
			rec(sir,f_desc);
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
