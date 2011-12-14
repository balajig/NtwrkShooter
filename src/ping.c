/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Muuss.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1989 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

/*
 *			P I N G . C
 *
 * Using the InterNet Control Message Protocol (ICMP) "ECHO" facility,
 * measure round-trip-delays and packet loss across network paths.
 *
 * Author -
 *	Mike Muuss
 *	U. S. Army Ballistic Research Laboratory
 *	December, 1983
 *
 * Status -
 *	Public Domain.  Distribution Unlimited.
 * Bugs -
 *	More statistics could always be gathered.
 *	This program has to run SUID to ROOT to access the ICMP socket.
 */

#include "ping_common.h"

#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#ifndef ICMP_FILTER
#define ICMP_FILTER	1
struct icmp_filter {
	__u32	data;
};
#endif


#define	MAXIPLEN	60
#define	MAXICMPLEN	76
#define	NROUTES		9		/* number of record route slots */
#define TOS_MAX		255		/* 8-bit TOS field */
#define MAX_HOSTNAMELEN	NI_MAXHOST


int settos = 0;			/* Set TOS, Precendence or other QOS options */
int icmp_sock;			/* socket file descriptor */
u_char outpack[0x10000];
int maxpacket = sizeof(outpack);

static int broadcast_pings = 0;

static char *pr_addr(__u32);
static u_short in_cksum(const u_short *addr, int len, u_short salt);
static void pr_icmph(__u8 type, __u8 code, __u32 info, struct icmphdr *icp);

static struct {
	struct cmsghdr cm;
	struct in_pktinfo ipi;
} cmsg = { {sizeof(struct cmsghdr) + sizeof(struct in_pktinfo), SOL_IP, IP_PKTINFO},
	   {0, }};
int cmsg_len;

char *device;
int pmtudisc = -1;
static u_char *packet;
static int packlen;

int ping_start (struct sockaddr_in where, struct if_info *device);


struct sockaddr_in whereto;

int ping_me (struct in_addr ip_addr)
{
	struct sockaddr_in where;

	memset (&cur_time, 0, sizeof(cur_time));
	memset (packet,  0, packlen);

	exiting = 0;
	ntransmitted  = 0;
	nreceived = 0;
	nrepeats = 0;
	nerrors = 0;
	nchecksum = 0;

	gettimeofday(&start_time, NULL);
	where.sin_family = AF_INET;
	where.sin_addr.s_addr = ip_addr.s_addr;

	return ping_start (where, NULL);
}


int setup_icmp_sock (void)
{
	int hold = 0;
	struct icmp_filter filt;

	icmp_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

	if (icmp_sock < 0)
		return -1;

	filt.data = ~((1<<ICMP_SOURCE_QUENCH)|
			(1<<ICMP_DEST_UNREACH)|
			(1<<ICMP_TIME_EXCEEDED)|
			(1<<ICMP_PARAMETERPROB)|
			(1<<ICMP_REDIRECT)|
			(1<<ICMP_ECHOREPLY));
	if (setsockopt(icmp_sock, SOL_RAW, ICMP_FILTER, (char*)&filt, sizeof(filt)) == -1)
		perror("WARNING: setsockopt(ICMP_FILTER)");

	if (setsockopt(icmp_sock, SOL_IP, IP_RECVERR, (char *)&hold, sizeof(hold)))
		fprintf(stderr, "WARNING: your kernel is veeery old. No problems.\n");

	/* Estimate memory eaten by single packet. It is rough estimate.
	 * Actually, for small datalen's it depends on kernel side a lot. */
	hold = datalen + 8;
	hold += ((hold+511)/512)* (20 + 16 + 64 + 160);
	sock_setbufs(icmp_sock, hold);

	setup(icmp_sock);

	return 0;
}

int ping_setup (void)
{

	packlen = datalen + MAXIPLEN + MAXICMPLEN;

	if (!(packet = (u_char *)malloc((u_int)packlen))) {
		fprintf(stderr, "ping: out of memory.\n");
		return -1;
	}

	return setup_icmp_sock ();	
}


int
ping_start (struct sockaddr_in where, struct if_info *device)
{
	int rval  = -1;

	if (device) {
		struct ifreq ifr;

		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, device->if_name, IFNAMSIZ-1);
		if (ioctl(icmp_sock, SIOCGIFINDEX, &ifr) < 0) {
			fprintf(stderr, "ping: unknown iface %s\n", device->if_name);
			exit(2);
		}
		cmsg.ipi.ipi_ifindex = ifr.ifr_ifindex;
		cmsg_len = sizeof(cmsg);
	}

	memcpy (&whereto, &where, sizeof(whereto));


	if (broadcast_pings) {
		if (setsockopt(icmp_sock, SOL_SOCKET, SO_BROADCAST,
			       &broadcast_pings, sizeof(broadcast_pings)) < 0) {
			perror ("ping: can't set broadcasting");
			exit(2);
		}
	}


	printf("PING  %s ", inet_ntoa(whereto.sin_addr));

	printf("%d(%d) bytes of data.\n", datalen, datalen+8+20);

	rval = main_loop(icmp_sock, packet, packlen);

	fflush (stdout);

	if (nreceived < ntransmitted) {
		fprintf (stderr, "\033[32mNTS : \033[0m\033[31m Packet loss : %ld%% .... \033[0m\n", 
			 nreceived ? (((ntransmitted - nreceived) * 100) / ntransmitted): 100);
	}

	return rval;
}


int receive_error_msg()
{
	int res;
	char cbuf[512];
	struct iovec  iov;
	struct msghdr msg;
	struct cmsghdr *cmsg;
	struct sock_extended_err *e;
	struct icmphdr icmph;
	struct sockaddr_in target;
	int net_errors = 0;
	int local_errors = 0;
	int saved_errno = errno;

	iov.iov_base = &icmph;
	iov.iov_len = sizeof(icmph);
	msg.msg_name = (void*)&target;
	msg.msg_namelen = sizeof(target);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;
	msg.msg_control = cbuf;
	msg.msg_controllen = sizeof(cbuf);

	res = recvmsg(icmp_sock, &msg, MSG_ERRQUEUE|MSG_DONTWAIT);
	if (res < 0)
		goto out;

	e = NULL;
	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
		if (cmsg->cmsg_level == SOL_IP) {
			if (cmsg->cmsg_type == IP_RECVERR)
				e = (struct sock_extended_err *)CMSG_DATA(cmsg);
		}
	}
	if (e == NULL)
		abort();

	if (e->ee_origin == SO_EE_ORIGIN_LOCAL) {
		local_errors++;
		if (e->ee_errno != EMSGSIZE)
			fprintf(stderr, "ping: local error: %s\n", strerror(e->ee_errno));
		else
			fprintf(stderr, "ping: local error: Message too long, mtu=%u\n", e->ee_info);
		nerrors++;
	} else if (e->ee_origin == SO_EE_ORIGIN_ICMP) {
		struct sockaddr_in *sin = (struct sockaddr_in*)(e+1);

		if (res < sizeof(icmph) ||
		    target.sin_addr.s_addr != whereto.sin_addr.s_addr ||
		    icmph.type != ICMP_ECHO ||
		    icmph.un.echo.id != ident) {
			/* Not our error, not an error at all. Clear. */
			saved_errno = 0;
			goto out;
		}

		acknowledge(ntohs(icmph.un.echo.sequence));

		if (!working_recverr) {
			struct icmp_filter filt;
			working_recverr = 1;
			/* OK, it works. Add stronger filter. */
			filt.data = ~((1<<ICMP_SOURCE_QUENCH)|
				      (1<<ICMP_REDIRECT)|
				      (1<<ICMP_ECHOREPLY));
			if (setsockopt(icmp_sock, SOL_RAW, ICMP_FILTER, (char*)&filt, sizeof(filt)) == -1)
				perror("\rWARNING: setsockopt(ICMP_FILTER)");
		}

		net_errors++;
		nerrors++;

		print_timestamp();
		printf("From %s icmp_seq=%u ", pr_addr(sin->sin_addr.s_addr), ntohs(icmph.un.echo.sequence));
		pr_icmph(e->ee_type, e->ee_code, e->ee_info, NULL);
		fflush(stdout);
	}

out:
	errno = saved_errno;
	return net_errors ? : -local_errors;
}

/*
 * pinger --
 * 	Compose and transmit an ICMP ECHO REQUEST packet.  The IP packet
 * will be added on by the kernel.  The ID field is our UNIX process ID,
 * and the sequence number is an ascending integer.  The first 8 bytes
 * of the data portion are used to hold a UNIX "timeval" struct in VAX
 * byte-order, to compute the round-trip time.
 */
int send_probe()
{
	struct icmphdr *icp;
	int cc;
	int i;

	icp = (struct icmphdr *)outpack;
	icp->type = ICMP_ECHO;
	icp->code = 0;
	icp->checksum = 0;
	icp->un.echo.sequence = htons(ntransmitted+1);
	icp->un.echo.id = ident;			/* ID */

	CLR((ntransmitted+1) % mx_dup_ck);

	cc = datalen + 8;			/* skips ICMP portion */

	/* compute ICMP checksum here */
	icp->checksum = in_cksum((u_short *)icp, cc, 0);


	do {
		static struct iovec iov = {outpack, 0};
		static struct msghdr m = { &whereto, sizeof(whereto),
						   &iov, 1, &cmsg, 0, 0 };
		m.msg_controllen = cmsg_len;
		iov.iov_len = cc;

		i = sendmsg(icmp_sock, &m, confirm);
		confirm = 0;
	} while (0);

	return (cc == i ? 0 : i);
}

/*
 * parse_reply --
 *	Print out the packet, if it came from us.  This logic is necessary
 * because ALL readers of the ICMP socket get a copy of ALL ICMP packets
 * which arrive ('tis only fair).  This permits multiple copies of this
 * program to be run without having intermingled output (or statistics!).
 */
void pr_echo_reply(__u8 *_icp, int len)
{
	struct icmphdr *icp = (struct icmphdr *)_icp;
	printf(" icmp_req=%u", ntohs(icp->un.echo.sequence));
}

int
parse_reply(struct msghdr *msg, int cc, void *addr, struct timeval *tv)
{
	struct sockaddr_in *from = addr;
	__u8 *buf = msg->msg_iov->iov_base;
	struct icmphdr *icp;
	struct iphdr *ip;
	int hlen;
	int csfailed;

	/* Check the IP header */
	ip = (struct iphdr *)buf;
	hlen = ip->ihl*4;
	if (cc < hlen + 8 || ip->ihl < 5) {
		return 1;
	}

	/* Now the ICMP part */
	cc -= hlen;
	icp = (struct icmphdr *)(buf + hlen);
	csfailed = in_cksum((u_short *)icp, cc, 0);

	if (icp->type == ICMP_ECHOREPLY) {
		if (icp->un.echo.id != ident)
			return 1;			/* 'Twas not our ECHO */
		if (gather_statistics((__u8*)icp, sizeof(*icp), cc,
				      ntohs(icp->un.echo.sequence),
				      ip->ttl, 0, tv, pr_addr(from->sin_addr.s_addr),
				      pr_echo_reply))
			return 0;
	} else {
		/* We fall here when a redirect or source quench arrived.
		 * Also this branch processes icmp errors, when IP_RECVERR
		 * is broken. */

		switch (icp->type) {
		case ICMP_ECHO:
			/* MUST NOT */
			return 1;
		case ICMP_SOURCE_QUENCH:
		case ICMP_REDIRECT:
		case ICMP_DEST_UNREACH:
		case ICMP_TIME_EXCEEDED:
		case ICMP_PARAMETERPROB:
			{
				struct iphdr * iph = (struct  iphdr *)(&icp[1]);
				struct icmphdr *icp1 = (struct icmphdr*)((unsigned char *)iph + iph->ihl*4);
				int error_pkt;
				if (cc < 8+sizeof(struct iphdr)+8 ||
				    cc < 8+iph->ihl*4+8)
					return 1;
				if (icp1->type != ICMP_ECHO ||
				    iph->daddr != whereto.sin_addr.s_addr ||
				    icp1->un.echo.id != ident)
					return 1;
				error_pkt = (icp->type != ICMP_REDIRECT &&
					     icp->type != ICMP_SOURCE_QUENCH);
				if (error_pkt) {
					acknowledge(ntohs(icp1->un.echo.sequence));
					if (working_recverr) {
						return 0;
					} else {
						static int once;
						/* Sigh, IP_RECVERR for raw socket
						 * was broken until 2.4.9. So, we ignore
						 * the first error and warn on the second.
						 */
						if (once++ == 1)
							fprintf(stderr, "\rWARNING: kernel is not very fresh, upgrade is recommended.\n");
						if (once == 1)
							return 0;
					}
				}
				nerrors+=error_pkt;
				print_timestamp();
				printf("From %s: icmp_seq=%u ",
				       pr_addr(from->sin_addr.s_addr),
				       ntohs(icp1->un.echo.sequence));
				if (csfailed)
					printf("(BAD CHECKSUM)");
				pr_icmph(icp->type, icp->code, ntohl(icp->un.gateway), icp);
				return !error_pkt;
			}
		default:
			/* MUST NOT */
			break;
		}
		printf("From %s: ", pr_addr(from->sin_addr.s_addr));
		if (csfailed) {
			printf("(BAD CHECKSUM)\n");
			return 0;
		}
		pr_icmph(icp->type, icp->code, ntohl(icp->un.gateway), icp);
		return 0;
	}

	putchar('\a');
	fflush(stdout);
	return 0;
}

u_short
in_cksum(const u_short *addr, register int len, u_short csum)
{
	register int nleft = len;
	const u_short *w = addr;
	register u_short answer;
	register int sum = csum;

	/*
	 *  Our algorithm is simple, using a 32 bit accumulator (sum),
	 *  we add sequential 16 bit words to it, and at the end, fold
	 *  back all the carry bits from the top 16 bits into the lower
	 *  16 bits.
	 */
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1)
		sum += htons(*(u_char *)w << 8);

	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return (answer);
}

/*
 * pr_icmph --
 *	Print a descriptive string about an ICMP header.
 */
void pr_icmph(__u8 type, __u8 code, __u32 info, struct icmphdr *icp)
{
	switch(type) {
	case ICMP_ECHOREPLY:
		printf("Echo Reply\n");
		/* XXX ID + Seq + Data */
		break;
	case ICMP_DEST_UNREACH:
		switch(code) {
		case ICMP_NET_UNREACH:
			printf("Destination Net Unreachable\n");
			break;
		case ICMP_HOST_UNREACH:
			printf("Destination Host Unreachable\n");
			break;
		case ICMP_PROT_UNREACH:
			printf("Destination Protocol Unreachable\n");
			break;
		case ICMP_PORT_UNREACH:
			printf("Destination Port Unreachable\n");
			break;
		case ICMP_FRAG_NEEDED:
			printf("Frag needed and DF set (mtu = %u)\n", info);
			break;
		case ICMP_SR_FAILED:
			printf("Source Route Failed\n");
			break;
		case ICMP_PKT_FILTERED:
			printf("Packet filtered\n");
			break;
		default:
			printf("Dest Unreachable, Bad Code: %d\n", code);
			break;
		}
		break;
	case ICMP_SOURCE_QUENCH:
		printf("Source Quench\n");
		break;
	case ICMP_REDIRECT:
		switch(code) {
		case ICMP_REDIR_NET:
			printf("Redirect Network");
			break;
		case ICMP_REDIR_HOST:
			printf("Redirect Host");
			break;
		case ICMP_REDIR_NETTOS:
			printf("Redirect Type of Service and Network");
			break;
		case ICMP_REDIR_HOSTTOS:
			printf("Redirect Type of Service and Host");
			break;
		default:
			printf("Redirect, Bad Code: %d", code);
			break;
		}
		if (icp)
			printf("(New nexthop: %s)\n", pr_addr(icp->un.gateway));
		break;
	case ICMP_ECHO:
		printf("Echo Request\n");
		/* XXX ID + Seq + Data */
		break;
	case ICMP_TIME_EXCEEDED:
		switch(code) {
		case ICMP_EXC_TTL:
			printf("Time to live exceeded\n");
			break;
		case ICMP_EXC_FRAGTIME:
			printf("Frag reassembly time exceeded\n");
			break;
		default:
			printf("Time exceeded, Bad Code: %d\n", code);
			break;
		}
		break;
	case ICMP_PARAMETERPROB:
		printf("Parameter problem: pointer = %u\n", icp ? (ntohl(icp->un.gateway)>>24) : info);
		break;
	case ICMP_TIMESTAMP:
		printf("Timestamp\n");
		/* XXX ID + Seq + 3 timestamps */
		break;
	case ICMP_TIMESTAMPREPLY:
		printf("Timestamp Reply\n");
		/* XXX ID + Seq + 3 timestamps */
		break;
	case ICMP_INFO_REQUEST:
		printf("Information Request\n");
		/* XXX ID + Seq */
		break;
	case ICMP_INFO_REPLY:
		printf("Information Reply\n");
		/* XXX ID + Seq */
		break;
#ifdef ICMP_MASKREQ
	case ICMP_MASKREQ:
		printf("Address Mask Request\n");
		break;
#endif
#ifdef ICMP_MASKREPLY
	case ICMP_MASKREPLY:
		printf("Address Mask Reply\n");
		break;
#endif
	default:
		printf("Bad ICMP type: %d\n", type);
	}
}

#if 0
void pr_options(unsigned char * cp, int hlen)
{
	int i, j;
	int optlen, totlen;
	unsigned char * optptr;
	static int old_rrlen;
	static char old_rr[MAX_IPOPTLEN];

	totlen = hlen-sizeof(struct iphdr);
	optptr = cp;

	while (totlen > 0) {
		if (*optptr == IPOPT_EOL)
			break;
		if (*optptr == IPOPT_NOP) {
			totlen--;
			optptr++;
			printf("\nNOP");
			continue;
		}
		cp = optptr;
		optlen = optptr[1];
		if (optlen < 2 || optlen > totlen)
			break;

		switch (*cp) {
		case IPOPT_SSRR:
		case IPOPT_LSRR:
			printf("\n%cSRR: ", *cp==IPOPT_SSRR ? 'S' : 'L');
			j = *++cp;
			i = *++cp;
			i -= 4;
			cp++;
			if (j > IPOPT_MINOFF) {
				for (;;) {
					__u32 address;
					memcpy(&address, cp, 4);
					cp += 4;
					if (address == 0)
						printf("\t0.0.0.0");
					else
						printf("\t%s", pr_addr(address));
					j -= 4;
					putchar('\n');
					if (j <= IPOPT_MINOFF)
						break;
				}
			}
			break;
		case IPOPT_RR:
			j = *++cp;		/* get length */
			i = *++cp;		/* and pointer */
			if (i > j)
				i = j;
			i -= IPOPT_MINOFF;
			if (i <= 0)
				break;
			if (i == old_rrlen
			    && !strncmp((char *)cp, old_rr, i))   {
				printf("\t(same route)");
				i = ((i + 3) / 4) * 4;
				cp += i;
				break;
			}
			old_rrlen = i;
			memcpy(old_rr, (char *)cp, i);
			printf("\nRR: ");
			cp++;
			for (;;) {
				__u32 address;
				memcpy(&address, cp, 4);
				cp += 4;
				if (address == 0)
					printf("\t0.0.0.0");
				else
					printf("\t%s", pr_addr(address));
				i -= 4;
				putchar('\n');
				if (i <= 0)
					break;
			}
			break;
		case IPOPT_TS:
		{
			int stdtime = 0, nonstdtime = 0;
			__u8 flags;
			j = *++cp;		/* get length */
			i = *++cp;		/* and pointer */
			if (i > j)
				i = j;
			i -= 5;
			if (i <= 0)
				break;
			flags = *++cp;
			printf("\nTS: ");
			cp++;
			for (;;) {
				long l;

				if ((flags&0xF) != IPOPT_TS_TSONLY) {
					__u32 address;
					memcpy(&address, cp, 4);
					cp += 4;
					if (address == 0)
						printf("\t0.0.0.0");
					else
						printf("\t%s", pr_addr(address));
					i -= 4;
					if (i <= 0)
						break;
				}
				l = *cp++;
				l = (l<<8) + *cp++;
				l = (l<<8) + *cp++;
				l = (l<<8) + *cp++;

				if  (l & 0x80000000) {
					if (nonstdtime==0)
						printf("\t%ld absolute not-standard", l&0x7fffffff);
					else
						printf("\t%ld not-standard", (l&0x7fffffff) - nonstdtime);
					nonstdtime = l&0x7fffffff;
				} else {
					if (stdtime==0)
						printf("\t%ld absolute", l);
					else
						printf("\t%ld", l - stdtime);
					stdtime = l;
				}
				i -= 4;
				putchar('\n');
				if (i <= 0)
					break;
			}
			if (flags>>4)
				printf("Unrecorded hops: %d\n", flags>>4);
			break;
		}
		default:
			printf("\nunknown option %x", *cp);
			break;
		}
		totlen -= optlen;
		optptr += optlen;
	}
}


/*
 * pr_iph --
 *	Print an IP header with options.
 */
void pr_iph(struct iphdr *ip)
{
	int hlen;
	u_char *cp;

	hlen = ip->ihl << 2;
	cp = (u_char *)ip + 20;		/* point to options */

	printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src      Dst Data\n");
	printf(" %1x  %1x  %02x %04x %04x",
	       ip->version, ip->ihl, ip->tos, ip->tot_len, ip->id);
	printf("   %1x %04x", ((ip->frag_off) & 0xe000) >> 13,
	       (ip->frag_off) & 0x1fff);
	printf("  %02x  %02x %04x", ip->ttl, ip->protocol, ip->check);
	printf(" %s ", inet_ntoa(*(struct in_addr *)&ip->saddr));
	printf(" %s ", inet_ntoa(*(struct in_addr *)&ip->daddr));
	printf("\n");
	pr_options(cp, hlen);
}
#endif

/*
 * pr_addr --
 *	Return an ascii host address as a dotted quad and optionally with
 * a hostname.
 */
char *
pr_addr(__u32 addr)
{
	struct hostent *hp;
	static char buf[4096];

	if (!(hp = gethostbyaddr((char *)&addr, 4, AF_INET)))
		sprintf(buf, "%s", inet_ntoa(*(struct in_addr *)&addr));
	else
		snprintf(buf, sizeof(buf), "%s (%s)", hp->h_name,
			 inet_ntoa(*(struct in_addr *)&addr));
	return(buf);
}


#include <linux/filter.h>

void install_filter(void)
{
	static int once;
	static struct sock_filter insns[] = {
		BPF_STMT(BPF_LDX|BPF_B|BPF_MSH, 0), /* Skip IP header. F..g BSD... Look into ping6. */
		BPF_STMT(BPF_LD|BPF_H|BPF_IND, 4), /* Load icmp echo ident */
		BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, 0xAAAA, 0, 1), /* Ours? */
		BPF_STMT(BPF_RET|BPF_K, ~0U), /* Yes, it passes. */
		BPF_STMT(BPF_LD|BPF_B|BPF_IND, 0), /* Load icmp type */
		BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, ICMP_ECHOREPLY, 1, 0), /* Echo? */
		BPF_STMT(BPF_RET|BPF_K, 0xFFFFFFF), /* No. It passes. */
		BPF_STMT(BPF_RET|BPF_K, 0) /* Echo with wrong ident. Reject. */
	};
	static struct sock_fprog filter = {
		sizeof insns / sizeof(insns[0]),
		insns
	};

	if (once)
		return;
	once = 1;

	/* Patch bpflet for current identifier. */
	insns[2] = (struct sock_filter)BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, htons(ident), 0, 1);

	if (setsockopt(icmp_sock, SOL_SOCKET, SO_ATTACH_FILTER, &filter, sizeof(filter)))
		perror("WARNING: failed to install socket filter\n");
}
