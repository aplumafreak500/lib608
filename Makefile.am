SUBDIRS = lib608
if ENABLE_FRONTEND
SUBDIRS += src
endif
dist_doc_DATA = License.txt
ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = configure
BUILT_SOURCES = $(top_srcdir)/.version
$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@
dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version
