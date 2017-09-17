/*! \file ghPing.c
	\brief	Use the ICMP protocol to request "echo" from destination
 */

/*
 * $Author: johan $
 * $Date: 2011/11/19 16:31:44 $
 * $Revision: 1.2 $
 * $State: Exp $
 *
 * $Log: ghPing.c,v $
 * Revision 1.2  2011/11/19 16:31:44  johan
 * Ping panel
 *
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.3  2011/02/10 11:09:42  johan
 * Take out CVS Id tag
 *
 * Revision 1.2  2011/02/03 08:38:15  johan
 * Add CVS stuff
 *
 *
 *
 */

#include <arpa/inet.h>

#include <stdlib.h>
#include <unistd.h>
#include <strings.h>

#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>

#include "ghPing.h"

#define PACKETSIZE	64
struct packet
{
	struct icmphdr hdr;
	char msg[PACKETSIZE-sizeof(struct icmphdr)];
};

int pid=-1;
struct protoent *proto=NULL;

extern int verbose;
extern int trace;

/*--------------------------------------------------------------------*/
/*--- checksum - standard 1s complement checksum                   ---*/
/*--------------------------------------------------------------------*/
unsigned short checksum(void *b, int len)
{	
	unsigned short *buf = b;
	unsigned int sum=0;
	unsigned short result;

	for ( sum = 0; len > 1; len -= 2 )
		sum += *buf++;
	if ( len == 1 )
		sum += *(unsigned char*)buf;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;
}

/*--------------------------------------------------------------------*/
/*--- display - present echo info                                  ---*/
/*--------------------------------------------------------------------*/
void display(void *buf, int bytes)
{
	int i;
	struct iphdr *ip = buf;
	struct icmphdr *icmp = buf+ip->ihl*4;

	printf("----------------\n");
	for ( i = 0; i < bytes; i++ )
	{
		if ( !(i & 15) ) printf("\n%X:  ", i);
		printf("%X ", ((unsigned char*)buf)[i]);
	}
	printf("\n");
	printf("IPv%d: hdr-size=%d pkt-size=%d protocol=%d TTL=%d ",
		ip->version, ip->ihl*4, ntohs(ip->tot_len), ip->protocol,
		ip->ttl);
#if 0
	printf("IPv%d: hdr-size=%d pkt-size=%d protocol=%d TTL=%d src=%s ",
		ip->version, ip->ihl*4, ntohs(ip->tot_len), ip->protocol,
		ip->ttl, inet_ntoa(ip->saddr));
	printf("dst=%s\n", inet_ntoa(ip->daddr));
#endif
	printf("\n");
	if ( icmp->un.echo.id == pid )
	{
		printf("ICMP: type[%d/%d] checksum[%d] id[%d] seq[%d]\n",
			icmp->type, icmp->code, ntohs(icmp->checksum),
			icmp->un.echo.id, icmp->un.echo.sequence);
	}
}

/*--------------------------------------------------------------------*/
/*--- listener - separate process to listen for and collect messages--*/
/*--------------------------------------------------------------------*/
void listener(void)
{
	int sd, cnt=1;
	struct sockaddr_in addr;
	unsigned char buf[1024];

	sd = socket(PF_INET, SOCK_RAW, proto->p_proto);
	if ( sd < 0 )
	{
		perror("socket");
		exit(0);
	}
	for (;;)
	{	int bytes, len=sizeof(addr);

		printf("Receive msg #%d\n", cnt);
		bzero(buf, sizeof(buf));
		bytes = recvfrom(sd, buf, sizeof(buf), MSG_DONTWAIT, (struct sockaddr*)&addr, &len);
		if ( bytes == 0 );
		else if ( bytes > 0 )
			display(buf, bytes);
		else
			perror("recvfrom");
		++cnt;
		sleep(1);
	}
	exit(0);
}

/*! \brief listen for message
 */
int listen1(void)
{
	int i, sd, cnt=1;
	struct sockaddr_in addr;
	unsigned char buf[1024];

	sd = socket(PF_INET, SOCK_RAW, proto->p_proto);
	if ( sd < 0 )
	{
		perror("socket");
	}
	else
	{	int bytes, len=sizeof(addr);

		for(i=0; i<1; i++)
		{
			sleep(1);
			if (trace) printf("Receive msg #%d\n", cnt);
			bzero(buf, sizeof(buf));
			bytes = recvfrom(sd, buf, sizeof(buf), MSG_DONTWAIT, (struct sockaddr*)&addr, &len);
			if ( bytes == 0 );
			else if ( bytes > 0 )
			{
				if (trace) display(buf, bytes);
				return 0;
			}
			else if (trace)
				perror("recvfrom");
			++cnt;
		}
	}
	return -1;
}

/*--------------------------------------------------------------------*/
/*--- ping - Create message and send it.                           ---*/
/*--------------------------------------------------------------------*/
void ping(struct sockaddr_in *addr)
{
	const int val=255;
	int i, sd, cnt=1;
	struct packet pckt;
	struct sockaddr_in r_addr;

	sd = socket(PF_INET, SOCK_RAW, proto->p_proto);
	if ( sd < 0 )
	{
		perror("socket");
		return;
	}
	if ( setsockopt(sd, SOL_IP, IP_TTL, &val, sizeof(val)) != 0)
		perror("Set TTL option");
	if ( fcntl(sd, F_SETFL, O_NONBLOCK) != 0 )
		perror("Request nonblocking I/O");
	for (;;)
	{	int len=sizeof(r_addr);

		printf("Send msg #%d\n", cnt);
		if ( recvfrom(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)&r_addr, &len) > 0 )
			printf("***Got message!***\n");
		bzero(&pckt, sizeof(pckt));
		pckt.hdr.type = ICMP_ECHO;
		pckt.hdr.un.echo.id = pid;
		for ( i = 0; i < sizeof(pckt.msg)-1; i++ )
			pckt.msg[i] = i+'0';
		pckt.msg[i] = 0;
		pckt.hdr.un.echo.sequence = cnt++;
		pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));
		if ( sendto(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)addr, sizeof(*addr)) <= 0 )
			perror("sendto");
		sleep(1);
	}
}

/*--------------------------------------------------------------------*/
/*--- ping - Create message and send it.                           ---*/
/*--------------------------------------------------------------------*/
void ping1(struct sockaddr_in *addr)
{
	const int val=255;
	int i, sd, cnt=1;
	struct packet pckt;
	struct sockaddr_in r_addr;
	int len=sizeof(r_addr);

	sd = socket(PF_INET, SOCK_RAW, proto->p_proto);
	if ( sd < 0 )
	{
		perror("socket");
		return;
	}
	if ( setsockopt(sd, SOL_IP, IP_TTL, &val, sizeof(val)) != 0)
		perror("Set TTL option");
	if ( fcntl(sd, F_SETFL, O_NONBLOCK) != 0 )
		perror("Request nonblocking I/O");

	if (trace) printf("Send msg #%d\n", cnt);
	if ( recvfrom(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)&r_addr, &len) > 0 )
	{
		if (trace) printf("***Got message!***\n");
	}
	bzero(&pckt, sizeof(pckt));
	pckt.hdr.type = ICMP_ECHO;
	pckt.hdr.un.echo.id = pid;
	for ( i = 0; i < sizeof(pckt.msg)-1; i++ )
		pckt.msg[i] = i+'0';
	pckt.msg[i] = 0;
	pckt.hdr.un.echo.sequence = cnt++;
	pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));
	if ( sendto(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)addr, sizeof(*addr)) <= 0 )
		perror("sendto");
}

int send_listen(char * ipAddr)
{
	int rc;
	struct hostent *hname;
	struct sockaddr_in addr;

	pid = getpid();
	proto = getprotobyname("ICMP");
	hname = gethostbyname(ipAddr);
	bzero(&addr, sizeof(addr));
	addr.sin_family = hname->h_addrtype;
	addr.sin_port = 0;
	addr.sin_addr.s_addr = *(long*)hname->h_addr;
	/*?
	if ( fork() == 0 )
		listener();
	else
		ping(&addr);
	wait(0);
	?*/
	ping1(&addr);
	rc = listen1();
	if (verbose)
	{
		if (rc) printf("Could not ping %s\n", ipAddr);
		else printf("Ping %s OK\n", ipAddr);
	}

	return rc;
}

