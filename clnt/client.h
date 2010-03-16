#ifndef CLIENT_H_
#define CLIENT_H_

struct nod
{
	char *name;
	struct nod *next;
};

typedef struct nod file_entry;

int startConnection(char*, int);

int readTree(int);

int removeOldFiles(char []);

int update(int);

int receiveFile(int, char*, int, int);

void filelistAdd(char *);

int filelistContains(char *);

#endif
