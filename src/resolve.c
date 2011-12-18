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

#define MAX_HOST                5
#define MAX_NAME_SERVERS	3
#define RESOLVE_CONF_FILE	"/etc/resolv.conf"

struct nameserver {
	struct in_addr namesrv_ip;;
};

static struct nameserver *name_servers = NULL;
static int ns_count = 0;

static int read_resolve_conf (void) 
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

			(name_servers +index)->namesrv_ip.s_addr = inet_addr(sub);

			nts_debug ("Name server IP Address %d  :  \"%s\"", index + 1, sub);

			ns_count++;

			if (++index >= MAX_NAME_SERVERS)
				break;
		}
	}

	if (line)
		free(line);
	return 0;
}

int check_ns_state (void) 
{
	int idx = 0;
	int no_resp = 0;
	
	while (idx < ns_count) {

		nts_debug ("Checking NAME server \"%s\" is alive or not .......", 
				inet_ntoa((name_servers + idx)->namesrv_ip));

		if (ping_me ((name_servers + idx)->namesrv_ip) < 0) {

			nts_debug ("Name server \"%s\" not responding .......", 
					inet_ntoa((name_servers + idx)->namesrv_ip));
			no_resp++;
		} else 
			nts_debug ("Name server \"%s\" is ALIVE :) .......\n", 
					inet_ntoa((name_servers + idx)->namesrv_ip));

		idx++;	
	}

	/*Sometimes default gateways acts as an DNS server, 
          some default gateways will not respond to ping*/

	if (try_to_resolve_host () < 0) {
		/*name servers not responding*/	
		if (no_resp == ns_count) {
			/*XXX:Why can't we try for public DNS servers? */
			fprintf (stderr, "\033[32mNTS : \033[0m\033[31mNameServers configured are down \033[0m\n");
		} else {
			fprintf (stderr, "\033[32mNTS : \033[0m\033[31mHostname lookup failed \033[0m\n");	
		}
		return -1;
	}

	/*Name servers are UP*/
	return 0;
}

int try_to_resolve_host (void)
{
	const char *host[MAX_HOST] = {
				      "yahoo.com", "google.com", 
				      "msn.com", "wikipedia.org", 
				      "github.com"
				     };

	int i = 0, resolved = -1;

	while (i < MAX_HOST) {

		nts_debug ("Trying resolve host \"%s\" .......",  host[i]);
		if (resolve_hostname (host[i]) < 0) {
			nts_debug ("Unable resolve host \"%s\" ....... :(",  host[i]);
		} else {
			resolved = 1;
			nts_debug ("Wow  host \"%s\" is resolved :) .......\n", host[i]);
		}

		i++;
	}

	return resolved;
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
		if (!ns_count)
			fprintf (stderr, "\033[32mNTS : \033[0m\033[31mName Servers not configured .... \033[0m\n");
		free (name_servers);
		return -1;
	}

	return 0;
}
