#
# Makefile.am for y2a_you/testsuite
#

AUTOMAKE_OPTIONS = dejagnu

plugindir = $(YAST2DIR)/plugin
libdir = $(YAST2DIR)/lib

INCLUDES = -I$(includedir)

noinst_PROGRAMS = runyou
runyou_SOURCES = runyou.cc

runyou_LDADD = -lscr -ly2 -lycp -lstdc++ -lcrypt
runyou_LDFLAGS = -L$(libdir)	\
	-Xlinker --whole-archive	\
	$(plugindir)/libpy2stdio.la	\
	$(plugindir)/libpy2script.la	\
	/usr/lib/libpkg.la		\
	../src/libpy2ag_you.la	\
	-Xlinker --no-whole-archive
