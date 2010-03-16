#ifndef CLIENT_H_
#define CLIENT_H_

struct nod
{
	char name[1024];
	struct nod *next;
};

int startConnection(char*, int);

int readTree(int);

int removeOldFiles();

int update(int);

int receiveFile(int, char*, int, int);

void filelistAdd(char[]);

int filelistContains(char *);

#endif