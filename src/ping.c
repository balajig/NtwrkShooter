#include "nt.h"
#include <linux/ip.h>
#include <linux/icmp.h>

unsigned short in_cksum(unsigned short *, int);

/*
 * in_cksum --
 * Checksum routine for Internet Protocol
 * family headers (C Version)
 */
unsigned short in_cksum(unsigned short *addr, int len)
{
	register int sum = 0;
	u_short answer = 0;
	register u_short *w = addr;
	register int nleft = len;
	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (nleft > 1)
	{
		sum += *w++;
		nleft -= 2;
	}
	/* mop up an odd byte, if necessary */
	if (nleft == 1)
	{
		*(u_char *) (&answer) = *(u_char *) w;
		sum += answer;
	}
	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);       /* add hi 16 to low 16 */
	sum += (sum >> 16);               /* add carry */
	answer = ~sum;              /* truncate to 16 bits */
	return (answer);
}

int ping_me (struct in_addr dst_addr)
{
	struct iphdr* ip;
	struct iphdr* ip_reply;
	struct icmphdr* icmp;
	struct sockaddr_in connection;
	char* packet;
	char* buffer;
	int sockfd;
	int optval;
	unsigned int addrlen;
	int siz;

	packet = malloc(sizeof(struct iphdr) + sizeof(struct icmphdr));
	buffer = malloc(sizeof(struct iphdr) + sizeof(struct icmphdr));

	ip = (struct iphdr *) packet;
	icmp = (struct icmphdr*) (packet + sizeof(struct iphdr));

	/*  
	 *  here the ip packet is set up
	 */
	ip->ihl          = 5;
	ip->version          = 4;
	ip->tos          = 0;
	ip->tot_len          = sizeof(struct iphdr) + sizeof(struct icmphdr);
	ip->id           = htons(0);
	ip->frag_off     = 0;
	ip->ttl          = 64;
	ip->protocol     = IPPROTO_ICMP;
	ip->saddr            = inet_addr ("10.10.10.173");
	ip->daddr            = dst_addr.s_addr;
	ip->check            = in_cksum((unsigned short *)ip, sizeof(struct iphdr));

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1)
	{
		perror("socket");
	//	exit(EXIT_FAILURE);
	}

	/* 
	 *  IP_HDRINCL must be set on the socket so that
	 *  the kernel does not attempt to automatically add
	 *  a default ip header to the packet
	 */

	setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &optval, sizeof(int));

	/*
	 *  here the icmp packet is created
	 *  also the ip checksum is generated
	 */
	icmp->type           = ICMP_ECHO;
	icmp->code           = 0;
	icmp->un.echo.id     = random();
	icmp->un.echo.sequence   = 0;
	icmp->checksum      = in_cksum((unsigned short *)icmp, sizeof(struct icmphdr));


	connection.sin_family = AF_INET;
	connection.sin_addr.s_addr = dst_addr.s_addr;

	/*
	 *  now the packet is sent
	 */

	while (1) {

		static 		int i = 0;
		sendto(sockfd, packet, ip->tot_len, 0, (struct sockaddr *)&connection, sizeof(struct sockaddr));
		printf("Sent %d byte packet to %s\n", ip->tot_len, inet_ntoa(dst_addr));

		/*
		 *  now we listen for responses
		 */
		addrlen = sizeof(connection);
		if (( siz = recvfrom(sockfd, buffer, sizeof(struct iphdr) + sizeof(struct icmphdr), 0, (struct sockaddr *)&connection, &addrlen)) == -1)
		{
			perror("recv");
		}
		else
		{
			printf("Received %d byte reply from %s:\n", siz , inet_ntoa(dst_addr));
			ip_reply = (struct iphdr*) buffer;
			printf("ID: %d\n", ntohs(ip_reply->id));
			printf("TTL: %d\n", ip_reply->ttl);
		}
		i++;
		if (i == 10)
			break;
	}
	free(packet);
	free(buffer);
	close(sockfd);
	return 0;
}

