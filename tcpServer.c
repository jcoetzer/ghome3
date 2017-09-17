/*
 ** tcpServer.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <libgen.h>
#include <popt.h>
#include <netinet/ip.h>
#include <ctype.h>

#include "tcpServer.h"
#include "dbHardware.h"

 /* port we're listening on */
#define PORT "9034"  

int trace = 0;
int verbose = 0;
int SendReply = 0;

int Timeout = 0;
unsigned short ServerPort = 0;      /*!< Server port */
char * ServerIP = NULL;             /*!< Server IP address (dotted quad) */

/*! \struct Command line parsing data structure */
struct poptOption optionsTable[] =
{
	{"gateway", 	'G', POPT_ARG_STRING, 	&ServerIP, 0,  "Listen address",  "<ip_address>"},
	{"port", 	'P', POPT_ARG_INT, 	&ServerPort, 0,  "Listen port(default is " PORT ")", "<integer>"},
	{"timeout", 	'T', POPT_ARG_INT, 	&Timeout, 0,  "Receive timeout period (default is 0)", "<integer>"},
	{"reply", 	'r', POPT_ARG_NONE, 	&SendReply, 0, "Send reply", NULL},
	{"trace", 	't', POPT_ARG_NONE, 	&trace, 0, "Trace/debug output", NULL},
	{"verbose", 	'v', POPT_ARG_NONE, 	&verbose, 0, "Verbose output", NULL},
	POPT_AUTOHELP
	{NULL,0,0,NULL,0}
};

struct ConnectionRec Connections[MAX_CONNECTNS];

/* Local functions */
void sighandle(int signum);
void *get_in_addr(struct sockaddr *sa);
void send_message(char * txbuf, int fdmax);

void read_msg(char * sendst, size_t txbytes);

/*! \brief Main functions
 * 	\param argc argument count
 * 	\param argv argument values
 */
int main(int argc, char *argv[])
{
	fd_set master;    			/*!< master file descriptor list */
	fd_set read_fds;  			/*!< temp file descriptor list for select() */
	int fdmax;        			/*!< maximum file descriptor number */
	int listener;     			/*!< listening socket descriptor */
	int newfd;        			/*!< newly accepted socket descriptor */
	struct sockaddr_storage remoteaddr; 	/*!< client address */
	socklen_t addrlen;
	char rxbuf[256];    			/*!< buffer for client data */
	char txbuf[256];    			/*!< buffer for client data */
	char porst[16];
	int nbytes;
	char remoteIP[INET6_ADDRSTRLEN];
	int yes=1;        			/*!< for setsockopt() SO_REUSEADDR, below */
	int i;
	struct addrinfo hints, *ai, *p;
	poptContext optcon;        		/*!< Context for parsing command line options */
	int rc;
	char * ipadr;

	signal(SIGINT, sighandle);
	signal(SIGQUIT, sighandle);
	signal(SIGPIPE, sighandle);
	
	for(i=0; i<MAX_CONNECTNS; i++)
	{
		Connections[i].IpAddr = NULL;
		Connections[i].Monitor = 0;
	}

	/* Read command line */
	optcon = poptGetContext(argv[0],argc,(const char **)argv,optionsTable,0);
	rc = poptGetNextOpt(optcon);
	if (rc < -1)
	{
		printf("%s: %s - %s\n", basename(argv[0]), poptBadOption(optcon,0),
		poptStrerror(rc));
		poptPrintUsage(optcon, stderr, 0);
		return -1;
	}
	poptFreeContext(optcon);
	
	if (! ServerPort)
		strcpy(porst, PORT);
	else
		sprintf(porst, "%d", ServerPort);

	FD_ZERO(&master);    			/* clear master and temp sets */
	FD_ZERO(&read_fds);

	/* Get socket and bind it */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rc = getaddrinfo(NULL, porst, &hints, &ai)) != 0) 
	{
		fprintf(stderr, "tcpServer: %s\n", gai_strerror(rc));
		exit(1);
	}

	for(p = ai; p != NULL; p = p->ai_next) 
	{
		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0) 
		{ 
			continue;
		}

		/* Suppress "address already in use" error message */
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) 
		{
			close(listener);
			continue;
		}

		break;
	}

	if (p == NULL) 
	{
		/* If we got here, it means we didn't get bound */
		fprintf(stderr, "tcpServer: failed to bind\n");
		exit(2);
	}

	freeaddrinfo(ai); 			/* all done with this */

	/* Prepare to accept connections on socket FD */
	if (listen(listener, 10) == -1)
	{
		fprintf(stderr, "ERROR: listen failed\n");
		exit(3);
	}

	/* Add the listener to the master set */
	FD_SET(listener, &master);

	/* Keep track of biggest file descriptor */
	fdmax = listener; 			/* so far, it's this one */

	/* Main loop */
	for(;;) 
	{
		/* Copy it */
		read_fds = master; 		
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) 
		{
			fprintf(stderr, "ERROR: select failed\n");
			exit(4);
		}

		/* Run through the existing connections looking for data to read */
		for(i = 0; i <= fdmax; i++) 
		{
			if (FD_ISSET(i, &read_fds)) 
			{
				/* we got one!! */
				if (i == listener) 
				{
					/* Handle new connections */
					addrlen = sizeof remoteaddr;
					newfd = accept(listener,
							(struct sockaddr *)&remoteaddr,
							&addrlen);
					if (newfd == -1) 
					{
						fprintf(stderr, "ERROR: accept failed\n");
					} 
					else 
					{
						ipadr = (char*) inet_ntop(remoteaddr.ss_family,
									get_in_addr((struct sockaddr*)&remoteaddr),
									remoteIP, 
									INET6_ADDRSTRLEN);
						/* add to master set */
						FD_SET(newfd, &master);
						if (newfd >= MAX_CONNECTNS)
							fprintf(stderr, "ERROR: Too many open sockets\n");
						else
						{
							Connections[newfd].IpAddr = strdup(ipadr);
							if (0==strcmp(ipadr, "127.0.0.1"))
								Connections[newfd].Monitor = 1;
							else
								Connections[newfd].Monitor = 0;
							Connections[newfd].Unit = getUnitIp(ipadr);
						}
						if (newfd > fdmax)
						{    
							/* Keep track of the maximum */
							fdmax = newfd;
						}
						sprintf(txbuf, "> New connection from %s (unit %d) on socket %d\n", 
							ipadr, Connections[newfd].Unit, newfd);
						send_message(txbuf, fdmax);
					}
				} 
				else 
				{
					/* Handle data from client */
					if ((nbytes = recv(i, rxbuf, sizeof(rxbuf), 0)) <= 0)
					{
						/* Error or connection closed by client */
						if (nbytes == 0) 
						{
							/* connection closed */
							sprintf(txbuf, "> Socket %d at %s disconnected\n", i, Connections[i].IpAddr);
							send_message(txbuf, fdmax);
						} 
						else 
						{
							fprintf(stderr, "ERROR: recv failed\n");
						}
						free(Connections[i].IpAddr);
						Connections[i].IpAddr = NULL;
						Connections[i].Monitor = 0;
						/* bye! */
						close(i); 
						/* Remove from master set */
						FD_CLR(i, &master); 
					}
					else 
					{
						/* Received valid data from client */
						read_msg(rxbuf, nbytes);
						sprintf(txbuf, "> Received from %d at %s (%d bytes)\n%s\n", 
							i, Connections[i].IpAddr, nbytes, rxbuf);
						send_message(txbuf, fdmax);
						dbHardware_AlarmLog(&Connections[i], rxbuf);
					}
				} /* END handle data from client */
			} /* END got new incoming connection */
		} /* END looping through file descriptors */
	} /* END main loop */
    
	return 0;
}

/*! \brief Convert message to readable format
 * 	\param sendst Data buffer
 * 	\param txbytes Buffer length
 */
void read_msg(char * sendst, size_t txbytes)
{
	int i;
	
	if (verbose) printf("DATA%d> ", (int) txbytes);
	for (i=0; i<txbytes; i++)
	{
		if (verbose) printf("%02x-", sendst[i]);
		if (! isprint(sendst[i]))
			sendst[i] = '.';
	}
	sendst[txbytes] = 0;
	if (verbose) printf("\n");	
}

/*! \brief Send message to monitor(s)
 * 	\param txbuf message string
 * 	\param fdmax maximum file descriptor
 */
void send_message(char * txbuf, int fdmax)
{
	int i;
	size_t txbytes;
	char sendst[256];
	
	if (verbose) printf("%s", txbuf);
	
	txbytes = strlen(txbuf);
	
	/* Loop through all active connections */
	for(i = 0; i <= fdmax; i++)
	{
		/* Check monitor flag */
		if (Connections[i].Monitor)
		{
			/* Send to FD */
			if (send(i, sendst, txbytes, 0) == -1)
			{
				fprintf(stderr, "ERROR: send failed\n");
			}
		}
	}
}

/*! \brief Get socket address, IPv4 or IPv6 
 * 	\param sa Socket address structure
 */
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*! \brief Signal handler
 *	\param	signum	signal number
 */
void sighandle(int signum)
{
	int i;
	
	/* Free memory used by strings */
	for(i=0; i<MAX_CONNECTNS; i++)
		if (Connections[i].IpAddr) free(Connections[i].IpAddr);
		
	printf("\nShutting down after signal %d\n", signum);
	
	exit(1);
}

