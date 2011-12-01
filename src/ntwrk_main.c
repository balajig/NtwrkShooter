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

#define VERSION  "0.1"

enum DEBUG {
	DEBUG_DISABLED = 0,
	DEBUG_ENABLED
};

static int nts_debug = DEBUG_DISABLED;

void show_license (void)
{
	fprintf (stdout, "This is free software: you are free to change and redistribute it.\n");
	fprintf	(stdout, "There is NO WARRANTY, to the extent permitted by law.\n\n");	
}

void show_version (void)
{
	fprintf (stdout, "Network Trouble Shooter (NTS) version %s\n", VERSION);
	show_license ();
	fflush  (stdout);
}

int main (int argc, char **argv)
{
	int options = 0;
	struct if_info    *p = NULL;

	/*TODO: Need to figure out the command line options*/

	while ((options = getopt(argc, argv, "d")) != -1) {
		switch (options) {
			case 'd':
				nts_debug = DEBUG_ENABLED;
				fprintf (stderr, "Debugging Mode enabled\n");
				break;
			default: 
				fprintf (stderr, "Invalid Option\n");
				return 0;
		}
	}

	show_version ();

	if (read_interfaces () < 0) {
		fprintf (stderr, "Unable to read the interface details\n");
		exit (1);
	}

	while ((p = get_next_if_info (p))) {
		if (!(p->admin_state & IFF_UP) &&  !(p->oper_state & IFF_RUNNING)) {
			if (make_if_up (p)  < 0) {
				/*Unable to make the interface up*/
				/*Del this interface from the list*/
				list_del (&p->nxt_if);
				fprintf (stderr, "\033[31mNTS: Unable to bring the Interface \"%s\" UP \033[0m\n", p->if_name);
			}
		}
	}
	
	if (!get_next_if_info (NULL)) {
		fprintf (stderr, "\033[31mNTS: All Interface on the system was DOWN \033[0m\n");
		return -1;
	}

	return 0;
}
