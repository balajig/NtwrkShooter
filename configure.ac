#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([NetworkTroubleShooter], [0.1], [BUG-REPORT-ADDRESS])
AM_INIT_AUTOMAKE(NetworkTroubleShooter,0.1) 
AC_OUTPUT(Makefile src/Makefile lib/Makefile)
AC_CONFIG_SRCDIR([src/ping_common.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB


# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h libintl.h locale.h netdb.h netinet/in.h stdlib.h string.h sys/file.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gethostbyaddr gettimeofday inet_ntoa memset setlocale socket strerror strrchr strstr])

AC_OUTPUT
