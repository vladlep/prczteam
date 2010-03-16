#ifndef _NETIO_H
#define _NETIO_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int set_addr(struct sockaddr_in *, char *, u_int32_t, short);

int stream_read(int, void *, int);

int stream_write(int, void *, int);

#endif
