AC_INIT([EIA-608 Tools], [m4_esyscmd([./git-version-gen.sh .tarball-version])], [], [608tools], [])
AM_INIT_AUTOMAKE([foreign dist-xz 1.11])
AC_CONFIG_SRCDIR(lib608/608.h)
AC_CONFIG_MACRO_DIRS([m4])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE([enable])
AC_ARG_ENABLE([frontend], AS_HELP_STRING([--disable-frontend], [Disable the frontend tools (scc2raw, raw2scc, ccasdi, etc.)]))
AM_CONDITIONAL([ENABLE_FRONTEND], [test "x$enable_frontend" != "xno"])
AC_PROG_CC
AM_PROG_AR
AC_PROG_INSTALL
LT_INIT([disable-shared, static, win32-dll])
AC_C_BIGENDIAN([AC_DEFINE(WORDS_BIGENDIAN, 1)], [AC_DEFINE(WORDS_BIGENDIAN, 0)])
AC_C_CONST
AC_C_VOLATILE
AX_FUNC_GETOPT_LONG
AC_CONFIG_HEADERS([lib608/config.h])
AC_CONFIG_FILES([
 Makefile
 src/Makefile
 lib608/Makefile
 lib608/lib608.pc
])
AC_OUTPUT
