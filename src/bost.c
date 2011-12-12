/*
   bost is a DNS resolving program, similar to the 'host' program(s).
   Copyright (C) 2006-2009  Patrik Lembke

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
   */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>

void check_host( const char *host );
int is_ip( const char *host );
void check_sub_ip( const char *ip );
void check_sub_host( const char *host );
int is_ip_range( const char *iprange );


static int double_check = 0;
static int ip_range_check = 0;

int resolve_hostname (const char *hostname)
{
	double_check = 1;
	ip_range_check = false;

	check_host (hostname);

	return 0;
}

void check_host( const char *host )
{
	struct addrinfo hints, *hostinfo, *iter;
	int hosts;
	char addr[50]; /* big enought for both IPv6: 46, IPv4: 16 */

	hosts = 0;
	memset( &hints, 0, sizeof( hints ) );
	hints.ai_family = AF_UNSPEC; /* So we can use this function for
					both types */
	hints.ai_socktype = SOCK_STREAM; /* Not sure why */

	if( !ip_range_check )
		printf( "%s:\n", host );

	if( getaddrinfo( host, NULL, &hints, &hostinfo ) != 0 )
	{
		if( !ip_range_check )
			printf( "\tLookup failed\n" );
	}
	else
	{
		for( iter = hostinfo; iter != NULL; iter = iter->ai_next )
		{
			switch( iter->ai_family )
			{
				case AF_INET: /* IPv4 */
					inet_ntop( AF_INET,
							&(( struct sockaddr_in *) iter->ai_addr)->sin_addr,
							addr, INET_ADDRSTRLEN );
					break;
				case AF_INET6: /* IPv6 */
					inet_ntop( AF_INET6,
							&(( struct sockaddr_in6 *) iter->ai_addr)->sin6_addr,
							addr, INET6_ADDRSTRLEN );
					break;
				default: /* Unknown */
					strncpy( addr, "unknown network family\0", 50 );
					break;
			}

			printf( "\t%d: %s\n", hosts, addr );

			if( double_check )
			{
				if( strncmp( "unknown", addr, 7 ) != 0 )
					check_sub_ip( addr );
			}

			hosts++;
		}

		freeaddrinfo( hostinfo ); /* don't be a memory glutton */
	}

	if( ip_range_check )
		printf( "\n" ); 
}

int is_ip( const char *ip )
{
	struct in6_addr test_ip;

	if( inet_pton( AF_INET, ip, &test_ip ) != false )
		return true; /* Its a IP */
	else if( inet_pton( AF_INET6, ip, &test_ip ) != false )
		return true; /* Its a IP */
	else
		return false; /* Its not */
}

int ip_type( const char *ip )
{
	int where;

	where = 0;

	while( ip[where] != '\0' )
	{
		if( ip[where] == '.' )
			return AF_INET;
		else if( ip[where] == ':' )
			return AF_INET6;

		where++;
	}

	return AF_UNSPEC;
}


void check_sub_ip( const char *ip )
{
	/* Used to check a sub ip.
	   (used if -d is specfied) */

	struct hostent *hostinfo;
	struct in_addr in_ipv4;
	struct in6_addr in_ipv6;

	int hosts; /* host counter */
	int type;

	/* Find out type */
	type = ip_type( ip );

	switch( type )
	{
		case AF_INET:
			inet_pton( AF_INET, ip, &in_ipv4 );
			hostinfo = gethostbyaddr( &in_ipv4, sizeof( in_ipv4 ), AF_INET );
			break;
		case AF_INET6:
			inet_pton( AF_INET6, ip, &in_ipv6 );
			hostinfo = gethostbyaddr( &in_ipv6, sizeof( in_ipv6 ), AF_INET6 );
			break;
		default:
			printf( "Error: %s is not a recogniced IP type\n", ip );
			exit( 1 );
			break;
	}

	hosts = 0;

	if( hostinfo == NULL )
	{
		if( !ip_range_check )
			printf( "\t\tLookup failed\n" );
	}
	else
	{
		printf( "\t\t%d: %s\n", hosts, hostinfo -> h_name );

		while( hostinfo -> h_aliases[hosts] != NULL )
		{
			printf( "\t\t%d: %s\n", hosts +1, hostinfo -> h_aliases[hosts] );
			hosts++;
		}
	}
}

void check_sub_host( const char *host )
{
	/* Used similarly to check_sub_ip */
	struct addrinfo hints, *hostinfo, *iter;
	int hosts;
	char addr[50]; /* big enought for both IPv6: 46, IPv4: 16 */

	hosts = 0;
	memset( &hints, 0, sizeof( hints ) );
	hints.ai_family = AF_UNSPEC; /* So we can use this function for
					both types */
	hints.ai_socktype = SOCK_STREAM; /* Not sure why */

	if( getaddrinfo( host, NULL, &hints, &hostinfo ) != 0 )
	{
		if( !ip_range_check )
			printf( "\t\tLookup failed\n" );
	}
	else
	{
		for( iter = hostinfo; iter != NULL; iter = iter->ai_next )
		{
			switch( iter->ai_family )
			{
				case AF_INET: /* IPv4 */
					inet_ntop( AF_INET,
							&(( struct sockaddr_in *) iter->ai_addr)->sin_addr,
							addr, INET_ADDRSTRLEN );
					break;
				case AF_INET6: /* IPv6 */
					inet_ntop( AF_INET6,
							&(( struct sockaddr_in6 *) iter->ai_addr)->sin6_addr,
							addr, INET6_ADDRSTRLEN );
					break;
				default: /* Unknown */
					strncpy( addr, "unknown network family\0", 50 );
					break;
			}

			printf( "\t\t%d: %s\n", hosts, addr );
			hosts++;
		}
	}

	freeaddrinfo( hostinfo ); /* don't be a memory glutton */

	if( ip_range_check )
		printf( "\n" ); 
}

int is_ip_range( const char *iprange )
{
	/* Checks if ip-range is a range
	   Currently no plans for IPv6 support here */
	int x, c;
	x = 0;

#ifdef DEBUG
	printf( "testing if '%s' is a ip range\n", iprange );
#endif

	while( iprange[x] != '\0' )
	{
		c = iprange[x];
		if( !( isdigit( c ) || c == '.' || c == '/' ))
			return false;
		x++;
	}
	return true;
}
