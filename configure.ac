#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

#AC_PREREQ(2.60)
AC_INIT(Serendipite, 0.1, paul@paul.cx)
AC_CONFIG_SRCDIR([json.c])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE(serendipite, main)

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_CHECK_PROGS(wget,[wget],no)
AC_CHECK_PROGS(tar,[tar],no)
AC_CHECK_PROGS(find,[find],no)

AC_ARG_ENABLE(debug,
AS_HELP_STRING([--enable-debug],
               [enable debugging, default: no]),
[case "${enableval}" in
             yes) debug=true ;;
             no)  debug=false ;;
             *)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
esac],
[debug=false])


AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")

AC_ARG_ENABLE(colors,
AS_HELP_STRING([--enable-colors],
               [enable colorsging, default: no]),
[case "${enableval}" in
             yes) colors=true ;;
             no)  colors=false ;;
             *)   AC_MSG_ERROR([bad value ${enableval} for --enable-colors]) ;;
esac],
[colors=false])

AM_CONDITIONAL(COLORS, test x"$colors" = x"true")

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
