#
# Makefile.am for y2a_you/testsuite
#

AUTOMAKE_OPTIONS = dejagnu

INCLUDES = -I$(includedir)

noinst_PROGRAMS = runyou
runyou_SOURCES = runyou.cc

runyou_LDADD = -lpkg -lscr -ly2 -lycp
runyou_LDFLAGS = -L$(libdir)		\
	-Xlinker --whole-archive	\
	$(plugindir)/libpy2stdio.la	\
	$(plugindir)/libpy2script.la	\
	../src/libpy2ag_you.la		\
	-Xlinker --no-whole-archive
