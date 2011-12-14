AM_CFLAGS = -Wall -g -O2 
bin_PROGRAMS = nshooter
nshooter_SOURCES = if.c ntwrk_main.c rt_info_nl.c ping.c ping_common.c resolve.c bost.c

nshooter_LDADD = ${top_builddir}/lib/libutil.a
nshooter_DEPENDENCIES = ${top_builddir}/lib/libutil.a

INCLUDES = -I${top_srcdir}/src -I${top_srcdir}/lib
