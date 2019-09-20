bin_PROGRAMS = raw2scc scc2raw
include_HEADERS = $(top_srcdir)/lib608/608.h
AM_CFLAGS = -I$(top_srcdir)/lib608/
raw2scc_SOURCES = raw2scc.c
scc2raw_SOURCES = scc2raw.c
EXTRA_DIST = gnugetopt.h
raw2scc_LDADD = $(top_srcdir)/lib608/lib608.la @LIBOBJS@ -lm
scc2raw_LDADD = $(top_srcdir)/lib608/lib608.la @LIBOBJS@ -lm