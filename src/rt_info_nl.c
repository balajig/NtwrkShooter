/* 
 *  Description: Network Trouble shooter 
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 */

#include "nt.h"
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#define RT_NL_DEBUG 1

static struct sockaddr_nl snl;
static int sock;

static struct rt_info *rtinfo;

static int rt_sock_create(void)
{
	int ret;
	unsigned long groups;
	sock = socket (AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	groups = RTMGRP_LINK | RTMGRP_IPV4_ROUTE | RTMGRP_IPV4_IFADDR;
#ifdef HAVE_IPV6
	groups |= RTMGRP_IPV6_ROUTE | RTMGRP_IPV4_IFADDR;
#endif
	if(sock < 0) {
	      fprintf (stderr, "Can't open socket\n");
	      exit(0);
	}
	
	memset (&snl, 0, sizeof snl);
	snl.nl_family = AF_NETLINK;
	snl.nl_groups = groups;

	/* Bind the socket to the netlink structure for anything. */
	ret = bind (sock, (struct sockaddr *) &snl, sizeof snl);
	if (ret < 0)
	{
	    fprintf (stderr, "Can't bind socket \n");
	    close (sock);
	    return -1;
	}
	return ret;
}

static int rt_table_read_req(int family)
{
	int ret,seq;
	struct sockaddr_nl snl;

	struct
	{
	    struct nlmsghdr nlh;
	    struct rtgenmsg g;
	} req;

	ret = seq = 0;

	if (sock < 0) {
	    fprintf (stderr, "socket isn't active.");
	    return -1;
	}

	memset (&snl, 0, sizeof snl);
	snl.nl_family = AF_NETLINK;

	memset (&req, 0, sizeof req);
	req.nlh.nlmsg_len = sizeof req;
	req.nlh.nlmsg_type = RTM_GETROUTE;
	req.nlh.nlmsg_flags = NLM_F_ROOT | NLM_F_MATCH | NLM_F_REQUEST;
	req.nlh.nlmsg_pid = getpid();
	req.nlh.nlmsg_seq = ++seq;
	req.g.rtgen_family = family;

	ret = sendto (sock, (void *) &req, sizeof req, 0,
		      (struct sockaddr *) &snl, sizeof snl);

	if (ret < 0) {
	    fprintf (stderr, "sendto failed: %s",strerror (errno));
	    return -1;
	  }
#ifdef RT_NL_DEBUG
	fprintf(stdout,"NLink Message sent \n");
#endif
	return 0;
}


static int populate_rt_info(struct prefix *p,  struct in_addr nh,
			    struct in_addr src, unsigned int ifindex,
			    unsigned int metric)
{
#ifdef RT_NL_DEBUG
        fprintf(stdout,"Response Details \n");
	fprintf(stdout,"Prefix: %s\n",inet_ntoa(p->u.prefix4));
#ifdef HAVE_IPV6
	fprintf(stdout,"Prefix: %s",inet_ntoa(p->u.prefix6));
#endif
	fprintf(stdout,"Gateway: %s\n",inet_ntoa(nh));
	fprintf(stdout,"Source: %s\n",inet_ntoa(src));
	fprintf(stdout,"Metric: %d\n",metric);
	fprintf(stdout,"Ifindex: %d\n",ifindex);
	fprintf(stdout,"\n");
#endif
	return 0;

}

static void netlink_parse_rtattr (struct rtattr **tb, int max, 
				  struct rtattr *rta, int len)
{
      while (RTA_OK (rta, len))
      {
	if (rta->rta_type <= max)
	  tb[rta->rta_type] = rta;
	rta = RTA_NEXT (rta, len);
      }
}

static int flush_rt_table(struct nlmsghdr *h)
{

      int len;
      struct rtmsg *rtm;
      struct rtattr *tb[RTA_MAX + 1];

      char anyaddr[16] = { 0 };

      int index;
      int metric;

      void *dest;
      struct in_addr gate;
      struct in_addr src;

      rtm = NLMSG_DATA (h);

      if (h->nlmsg_type != RTM_NEWROUTE)
	return 0;
      if (rtm->rtm_type != RTN_UNICAST)
	return 0;

      len = h->nlmsg_len - NLMSG_LENGTH (sizeof (struct rtmsg));
      if (len < 0)
	return -1;

      memset (tb, 0, sizeof tb);
      netlink_parse_rtattr (tb, RTA_MAX, RTM_RTA (rtm), len);

      if (rtm->rtm_src_len != 0)
	return 0;

      index = 0;
      metric = 0;
      dest = NULL;

      if (tb[RTA_OIF])
	index = *(int *) RTA_DATA (tb[RTA_OIF]);

      if (tb[RTA_DST])
	dest = RTA_DATA (tb[RTA_DST]);
      else
	dest = anyaddr;

      if (tb[RTA_PREFSRC])
	src = *(struct in_addr *) RTA_DATA (tb[RTA_PREFSRC]);

      if (tb[RTA_GATEWAY])
	gate = *(struct in_addr *) RTA_DATA (tb[RTA_GATEWAY]);

      if (tb[RTA_PRIORITY])
	metric = *(int *) RTA_DATA(tb[RTA_PRIORITY]);

      if (rtm->rtm_family == AF_INET)
      {
	  struct prefix p;
	  p.family = AF_INET;
	  memcpy (&p.u.prefix4, dest, 4);
	  p.prefixlen = rtm->rtm_dst_len;

         /*
	  * Store the info into the local structure
	  */
	 populate_rt_info(&p, gate, src, index, metric);
      }
#ifdef HAVE_IPV6
      if (rtm->rtm_family == AF_INET6)
      {
	  struct prefix p;
	  p.family = AF_INET6;
	  memcpy (&p.u.prefix, dest, 16);
	  p.prefixlen = rtm->rtm_dst_len;

         /*
	  * Store the info into the local structure
	  */
      }
#endif /* HAVE_IPV6 */

      return 0;
}


int parse_response(void)
{
      int status;
      int ret = 0;

      while (1)
	{
	  char buf[4096];
	  struct iovec iov = { buf, sizeof buf };
	  struct sockaddr_nl snl;
	  struct msghdr msg = { (void *) &snl, sizeof snl, &iov, 1, NULL, 0, 0 };
	  struct nlmsghdr *h;

	  status = recvmsg (sock, &msg, 0);
	  if (status < 0)
	   {
	      if (errno == EINTR)
		continue;
	      if (errno == EWOULDBLOCK || errno == EAGAIN)
		break;
	      fprintf (stderr, "recvmsg overrun: %s", strerror(errno));
	      continue;
	    }

	  if (status == 0)
	    {
	      fprintf(stderr, "EOF");
	      return -1;
	    }

	  if (msg.msg_namelen != sizeof snl)
	    {
	      fprintf (stderr, "sender address length error\n ");
	      return -1;
	    }
	  
	  for (h = (struct nlmsghdr *) buf; NLMSG_OK (h, (unsigned int) status);
	       h = NLMSG_NEXT (h, status))
	    {
	      /* Finish of reading. */
	      if (h->nlmsg_type == NLMSG_DONE)
		return ret;

	      /* Error handling. */
	      if (h->nlmsg_type == NLMSG_ERROR)
		{
		  struct nlmsgerr *err = (struct nlmsgerr *) NLMSG_DATA (h);

		  /* If the error field is zero, then this is an ACK */
		  if (err->error == 0)
		    {
			  fprintf (stderr,"%s: type=%d, seq=%u, pid=%u", \
				     __FUNCTION__, \
				     err->msg.nlmsg_type, err->msg.nlmsg_seq,\
				     err->msg.nlmsg_pid);
		    }

		      /* return if not a multipart message, otherwise continue */
		      if (!(h->nlmsg_flags & NLM_F_MULTI))
			{
			  return 0;
			}
		      continue;
		    }

		  if (h->nlmsg_len < NLMSG_LENGTH (sizeof (struct nlmsgerr)))
		    {
		      fprintf (stderr, "error: message truncated");
		      return -1;
		    }

	      //call my function here 
	      flush_rt_table(h);
	    }
	}
      return ret;
}

int rtnl_init(void)
{
  	rt_sock_create();
	rt_table_read_req(AF_INET);
	parse_response();
	return 0;

}


