#
# Makefile.am for y2a_you/src
#

plugin_LTLIBRARIES = libpy2ag_you.la
lib_LTLIBRARIES = liby2ag_you.la

liby2ag_you_la_SOURCES = 			\
	PatchInfo.cc PatchInfo.h		\
	YouAgentCalls.cc YouAgentCalls2.cc YouAgentCalls.h \
	YouAgent.cc YouAgent.h
liby2ag_you_la_LDFLAGS = -version-info 2:0

libpy2ag_you_la_SOURCES =			\
	$(liby2ag_you_la_SOURCES)		\
	Y2CCYou.cc 	Y2CCYou.h		\
	Y2YouComponent.cc Y2YouComponent.h
libpy2ag_you_la_LDFLAGS = -version-info 2:0

libpy2ag_you_la_LIBADD =			\
	-lpkg
liby2ag_you_la_LIBADD = $(libpy2ag_you_la_LIBADD)

INCLUDES = -I$(includedir)

