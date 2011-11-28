struct if_info {
	struct list_head nxt_if;
	char		if_name[MAX_IF_NAME];
	uint32_t	ipv4_address;  /*XXX: Currently our focus is only on IPV4 address*/
	int		sock_fd;
	int		if_idx;
	int		link_state;	/*IFF_UP or IFF_DOWN*/

};

