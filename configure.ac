AC_COPYRIGHT([Copyright 2009 the librandomcrash team])

AC_INIT(librandomcrash, 0.0.0)
AC_PREREQ(2.50)
AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE

LT_MAJOR="0"
LT_MINOR="0"
LT_MICRO="0"
AC_SUBST(LT_MAJOR)
AC_SUBST(LT_MINOR)
AC_SUBST(LT_MICRO)
LT_VERSION="$LT_MAJOR:$LT_MINOR:$LT_MICRO"
AC_SUBST(LT_VERSION)

AC_PROG_CC
AC_PROG_EGREP

m4_undefine([AC_PROG_F77])
m4_defun([AC_PROG_F77],[])

AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

case "${host_os}" in
    "")
      SYS=unknown
      ;;
    linux*)
      SYS=linux
      ;;
    darwin*)
      SYS=darwin
      ;;
    *)
      SYS="${host_os}"
    ;;
esac

AM_CONDITIONAL(HAVE_DARWIN, [test "${SYS}" = "darwin"])
AM_CONDITIONAL(HAVE_LINUX, [test "${SYS}" = "linux"])

AC_ARG_WITH(debug,
		AS_HELP_STRING([--with-debug],
		               [enables hacker mode]),
			       [debug="-D__LRC_DEBUG__"],
			       [debug="-U__LRC_DEBUG__"])
AC_SUBST(debug)

SYMBOL_PREFIX="lrc_"
AC_DEFINE_UNQUOTED(SYMBOL_PREFIX, "${SYMBOL_PREFIX}", [Define to prefix exported library symbols])
AC_SUBST(SYMBOL_PREFIX)

AC_CONFIG_FILES([Makefile] [src/librandomcrash.pc] [src/Makefile] [launcher/Makefile])
AC_OUTPUT

