#ifndef NT_H
#define NT_H
/* 
 *  Description: Network Trouble shooter 
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <locale.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <linux/sockios.h>
#include <libintl.h>
#include "list.h"
#include "progname.h"

#define PACKAGE "NetworkManager"

/*XXX: It should be auto generated one*/
#define LOCALEDIR "/usr/local/share/locale"


enum GET_IF {
	GET_IF_BY_NAME = 1, 
	GET_IF_BY_IPADDR,
	GET_IF_BY_IFINDEX
};


struct if_info {
	struct list_head nxt_if;
	char		if_name[IFNAMSIZ];
	unsigned int	ipv4_address;  /*XXX: Currently our focus is only on IPV4 address*/
	unsigned int    ipv4_netmask;
	int		sock_fd;
	int		if_idx;
	int		admin_state;	/*IFF_UP or IFF_DOWN*/
	int		oper_state;   /*IFF_RUNNING*/
	int 		flags;
};


/**
 *	struct prefix - prefix structure
 *	@family:  Denotes whether its IPv4 or Ipv6
 *	@prefixlen: Denotes the subnet mask
 *	@prefix: Denotes the IPv4 Prefix 
 *	@prefix6: Denotes the IPv6 Prefix
 */
struct prefix {   
	unsigned char family; 
	unsigned char prefixlen;
	union
	{
	  struct in_addr prefix4;
    #ifdef HAVE_IPV6
	  struct in6_addr prefix6;
    #endif /* HAVE_IPV6 */
	} u;
};


/**
 *	struct rt_info - route information structure
 * 	@p: pointer to struct prefix
 * 	@distance: admin distance for the route
 * 	@flags: Denotes the flag for the route
 * 	@nexthop: nexthop address
 *	@ifname: interface name
 *      @rt_list: list of all route information 
 */
struct rt_info {
	struct prefix *p;
	unsigned char distance;	
	unsigned char flags;
	struct in_addr nexthop;
	struct if_info *ifname;
        struct list_head  rt_list; //FIXME - Need to think about a better way probably a hash method to lookup instead of lists. 
};

struct if_info * get_next_if_info (struct if_info *p);
int make_if_up (struct if_info *p);
void display_interface_info (void);
int  read_interfaces (void);
int rtnl_init(void);
int nts_debug (char *fmt, ...);
void set_debug_enable (void);

#endif  /* NT_H */
