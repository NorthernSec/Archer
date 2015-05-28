#ifndef MAIN_H
#define MAIN_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

/* === DEFINITIONS === */
#define	NUM_ATTACKS	3

/* === DATA STRUCTURES === */
struct attackTypes {
	char		*type;
	void		(*f)();
};

/* === FUNCTION PROTOTYPES === */
void			showUsage();
void			httpGet(void *);
void			httpKeepalive(void *);
void			syn(void *);

/* === GLOBAL VARIABLES === */
int			sendCount, readCount, reloadCount, port, sockets;
pthread_mutex_t		mutex = PTHREAD_MUTEX_INITIALIZER;
struct attackTypes	attacks[] = {
	{"http-get", httpGet},
	{"syn", syn}
};

#endif
