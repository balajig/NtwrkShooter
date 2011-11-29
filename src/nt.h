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
#include <errno.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <linux/sockios.h>
#include "list.h"

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
