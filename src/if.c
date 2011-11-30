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
#include <sys/ioctl.h>

#define _PATH_PROCNET_DEV "/proc/net/dev"


LIST_HEAD(if_hd);

static struct if_info *add_if_info(char *name)
{
    struct if_info * new = calloc (1, sizeof (struct if_info));

    if (!new) {
	return NULL;
    }

    strncpy(new->if_name, name, IFNAMSIZ);

    list_add_tail (&new->nxt_if, &if_hd);

    return new;
}
static char *get_name(char *name, char *p)
{
    while (isspace(*p))
        p++;
    while (*p) {
        if (isspace(*p))
            break;
        if (*p == ':') {        /* could be an alias */
            char *dot = p, *dotname = name;
            *name++ = *p++;
            while (isdigit(*p))
                *name++ = *p++;
            if (*p != ':') {    /* it wasn't, backup */
                p = dot;
                name = dotname;
            }
            if (*p == '\0')
                return NULL;
            p++;
            break;
        }
        *name++ = *p++;
    }
    *name++ = '\0';
    return p;
}


static int if_readconf(void)
{
	int numreqs = 30;
	struct ifconf ifc;
	struct ifreq *ifr;
	int n, err = -1;
	int   skfd      = socket(AF_INET, SOCK_DGRAM, 0);

	if (skfd < 0)
		return (-1);

	ifc.ifc_buf = NULL;
	for (;;) {
		ifc.ifc_len = sizeof(struct ifreq) * numreqs;
		ifc.ifc_buf = realloc(ifc.ifc_buf, ifc.ifc_len);

		if (ioctl(skfd, SIOCGIFCONF, &ifc) < 0) {
			perror("SIOCGIFCONF");
			goto out;
		}
		if (ifc.ifc_len == sizeof(struct ifreq) * numreqs) {
			/* assume it overflowed and try again */
			numreqs += 10;
			continue;
		}
		break;
	}

	ifr = ifc.ifc_req;
	for (n = 0; n < ifc.ifc_len; n += sizeof(struct ifreq)) {
		add_if_info(ifr->ifr_name);
		ifr++;
	}
	err = 0;

out:
	free(ifc.ifc_buf);
	return err;
}

static int if_readlist_proc(char *target)
{
	static int proc_read;
	FILE *fh;
	char buf[512];
	struct if_info *ife;
	int err;

	if (proc_read)
		return 0;
	if (!target)
		proc_read = 1;

	fh = fopen(_PATH_PROCNET_DEV, "r");
	if (!fh) {
		fprintf(stderr, ("Warning: cannot open %s (%s). Limited output.\n"),
				_PATH_PROCNET_DEV, strerror(errno));
		return if_readconf();
	}
	fgets(buf, sizeof buf, fh); /* eat line */
	fgets(buf, sizeof buf, fh);

	//procnetdev_vsn = procnetdev_version(buf);

	err = 0;
	while (fgets(buf, sizeof buf, fh)) {
		char *s, name[IFNAMSIZ];
		s = get_name(name, buf);
		ife = add_if_info(name);
		if (target && !strcmp(target,name))
			break;
	}
	if (ferror(fh)) {
		perror(_PATH_PROCNET_DEV);
		err = -1;
		proc_read = 0;
	}

	fclose(fh);
	return err;
}

int if_readlist(void)
{
	int err = if_readlist_proc (NULL);
	if (err < 0)
		err = if_readconf();
	return err;
}

int fetch_and_update_if_info (struct if_info *ife)
{
	struct ifreq ifr;
	char *ifname = ife->if_name; 
	int  fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (fd < 0)
		return (-1);

	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	if (ioctl(fd, SIOCGIFADDR, (char *)&ifr) == 0) {

		struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;
		ife->ipv4_address = ntohl(sin->sin_addr.s_addr);

		if (ioctl(fd, SIOCGIFNETMASK, &ifr) == 0) {
			struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;
			ife->ipv4_netmask = ntohl(sin->sin_addr.s_addr);
		}

	} 	

	if (ioctl(fd, SIOCGIFINDEX, (char *)&ifr) == 0)
		ife->if_idx = ifr.ifr_ifindex;

	if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0)  {
		ife->admin_state = ifr.ifr_flags & IFF_UP;
		ife->oper_state  = ifr.ifr_flags & IFF_RUNNING;
	} 

	close(fd);

	return 0;
#if 0

	if (errno == ENODEV) { 
	    errmsg = ("Device not found"); 
	} else { 
	    errmsg = strerror(errno); 
	}

  	fprintf(stderr, ("%s: error fetching interface information: %s\n"),
		ife->if_name, errmsg);
#endif
	return -1;
}

int read_interfaces (void)
{
        struct if_info    *p = NULL;
        struct list_head  *head = &if_hd;

	if (if_readlist () < 0)
		return -1;

        list_for_each_entry (p, head, nxt_if) {
		if (fetch_and_update_if_info (p) < 0)
			return -1;
        }

	return 0;
}

struct if_info * get_next_if_info (struct if_info *p)
{
	struct list_head *nxt = NULL;
	struct if_info *nxtif = NULL;

	if (!p) {
		/*if p is NULL , get the first entry in the list*/
		nxt = &if_hd;
		nxtif = list_first_entry (nxt, struct if_info, nxt_if);
		
	} else {
		nxt = &p->nxt_if;
		if (nxt->next != &if_hd)
			nxtif = list_entry (nxt->next, struct if_info, nxt_if);
	}

	return nxtif;
}

/*Following routies just to verify the database*/
void display_interface_info (void)
{

        struct if_info    *p = NULL;
        struct list_head  *head = &if_hd;

	fprintf (stdout, "\nIf_name     If_Index      If_addess    If_netmask    If_adminstate    If_operstate\n");
	fprintf (stdout, "-------     --------      ----------   ----------    -------------    ------------\n");

        list_for_each_entry (p, head, nxt_if) {
		printf ("%-10s   %-10d   %-10x   %-10x  %10s   %10s\n", p->if_name, p->if_idx, p->ipv4_address, p->ipv4_netmask, 
		        (p->admin_state & IFF_UP)?"UP": "DOWN", (p->oper_state & IFF_RUNNING)?"UP":"DOWN");
        }
}

void display_interface_info_get_next_test (void)
{

        struct if_info    *p = NULL;

	fprintf (stdout, "If_name     If_Index      If_addess    If_netmask    If_adminstate    If_operstate\n");
	fprintf (stdout, "-------     --------      ----------   ----------    -------------    ------------\n");

        while ((p = get_next_if_info (p))) {
		printf ("%-10s   %-10d   %-10x   %-10x  %10s   %10s\n", p->if_name, p->if_idx, p->ipv4_address, p->ipv4_netmask, 
		        (p->admin_state & IFF_UP)?"UP": "DOWN", (p->oper_state & IFF_RUNNING)?"UP":"DOWN");
        }
}
