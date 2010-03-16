#ifndef SERVER_H_
#define SERVER_H_

int startServer(int);

void acceptConnections();

void acceptMessages(int);

int sendFile(int, char *, int);

static void updateServerTree(int);

int writeTree();

void createTree(char[], int);

#endif