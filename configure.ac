#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([ringmaster], [1.0], [francisy@microsoft.com])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_HEADERS([config.h])

CXX17_FLAGS="-std=c++17 -pthread"
PICKY_CXXFLAGS="-pedantic -Wall -Wextra -Werror -Weffc++"
AC_SUBST([CXX17_FLAGS])
AC_SUBST([PICKY_CXXFLAGS])

# Checks for programs.
AC_PROG_CXX
AC_PROG_RANLIB

# Checks for libraries.
PKG_CHECK_MODULES([VPX],[vpx])
PKG_CHECK_MODULES([SDL],[sdl2])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([
    Makefile
    src/Makefile
    src/util/Makefile
    src/video/Makefile
    src/app/Makefile
])

AC_OUTPUT
