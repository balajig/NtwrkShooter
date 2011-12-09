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

void show_usage (void)
{
	fprintf (stdout, "Usage: nshooter [OPTION]\n");
	fprintf (stdout, "Troubleshoots the network and reports problem\n");
	fprintf (stdout, "Options:\n");
	fprintf (stdout, "-d,                   Enables debugging mode\n");
	fprintf (stdout, "-h,                   Displays help message\n");
}

int main (int argc, char **argv)
{
	int options = 0;
	struct if_info    *p = NULL, *tmp = NULL;

	set_program_name (argv[0]);
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	show_version ();

	/*TODO: Need to figure out the command line options*/

	while ((options = getopt(argc, argv, "dh")) != -1) {
		switch (options) {
			case 'd':
				fprintf (stderr, "NTS: Debugging Mode enabled\n\n");
				set_debug_enable ();
				break;
			case 'h':
				show_usage ();
				exit (1);
				break;
			default: 
				fprintf (stderr, "Invalid Option\n");
				return 0;
		}
	}

	if (read_interfaces () < 0) {
		fprintf (stderr, "Unable to read the interface details\n");
		exit (1);
	}

	while ((p = get_next_if_info (p))) {
		if (tmp) {
			free (tmp);
			tmp = NULL;
		}
		if (!p->admin_state &&  !p->oper_state) {
			if (make_if_up (p)  < 0) {
				/*Unable to make the interface up*/
				/*Del this interface from the list*/
				list_del (&p->nxt_if);
				nts_debug ("Unable to bring the Interface \"%s\" UP \n", p->if_name);
				tmp = p;
			}
		}
	}
	
	if (!get_next_if_info (NULL)) {
		fprintf (stderr, "\033[32mNTS : \033[0m\033[31mAll Interface on the system was DOWN \033[0m\n");
		return -1;
	}

	if (ping_setup () < 0) {
		fprintf (stderr, "\033[32mNTS : \033[0m\033[31mUnable to setup sock for ping \033[0m\n");
		return -1;
	}


	rtnl_init();

	if (resolver_init () < 0) {
		return -1;
	}


	if (check_ns_state () < 0) {
		fprintf (stderr, "\033[32mNTS : \033[0m\033[31mNameServers configured are down \033[0m\n");
		return -1;
	}

	return 0;
}
