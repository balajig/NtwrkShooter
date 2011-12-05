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
#include <stdarg.h>

#define DEBUG_DISABLED    0
#define DEBUG_ENABLED    1


static int nts_dbg = DEBUG_DISABLED;

void set_debug_enable (void)
{
	nts_dbg = DEBUG_ENABLED;
}


int nts_debug (char *fmt , ...)
{
	if (nts_dbg) {
		va_list v_args;
		fprintf (stdout, "\033[32mNTS : \033[0m");
		va_start (v_args, fmt);
		vprintf(fmt, v_args);
		va_end(v_args);
		fprintf (stdout, "\n");
	}

	return 0;
}
