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


void show_version (void)
{
	fprintf (stdout, "Network Trouble Shooter %s\n", VERSION);
	fflush  (stdout);
}

int main (int argc, char **argv)
{
	int options = 0;

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
}
