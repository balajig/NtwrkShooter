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

#define MAX_NAME_SERVERS	3
#define RESOLVE_CONF_FILE	"/etc/resolv.conf"

struct nameserver {
	uint32_t namesrv_ip;
};

static struct nameserver *name_servers = NULL;

int read_resolve_conf (void) 
{
	char *    line = NULL;
	size_t    len = 0;
	ssize_t   read;
	int       index = 0;
	FILE *fp = fopen (RESOLVE_CONF_FILE, "r");

	if (!fp) {
		fprintf (stderr, "NTS: Unable to open \"%s\" ...", RESOLVE_CONF_FILE);
		return -1;
	}

	while ((read = getline(&line, &len, fp)) != -1) {

		/*Parse and extract the name server IP address*/

		char *sub = strstr (line, "nameserver ");

		if (sub) {
			int len = 0;

			sub += strlen ("nameserver ");

			len = strlen(sub);

			/*truncate newline char*/
			if (sub[len -1] == '\n')
				sub[len - 1] = '\0';

			(name_servers +index)->namesrv_ip = ntohl(inet_addr(sub));

			nts_debug ("Name server IP Address %d  :  \"%x\"", index + 1, 
					(name_servers + index)->namesrv_ip);

			if (++index >= MAX_NAME_SERVERS)
				break;
		}
	}

	if (line)
		free(line);
	return 0;
}

/* Return the ip address if host provided by name
 * Sasi: I Hope it is not required for reading namesevers ?*/

char * to_ip_addr (char * addr)
{
	struct hostent * host = NULL;

	host = gethostbyname(addr);

	if (host)
		return inet_ntoa(*(struct in_addr *)host->h_addr);
	else
		return NULL;
}


int resolver_init (void)
{
	int alloc_len = sizeof (struct nameserver) * MAX_NAME_SERVERS;

	name_servers = malloc (alloc_len);

	if (!name_servers) {
		fprintf (stderr, "NTS: Unable to allocate memory .....\n");
		return -1;
	}

	memset (name_servers, 0, alloc_len);

	if (read_resolve_conf () < 0) {
		free (name_servers);
		return -1;
	}

	return 0;
}
