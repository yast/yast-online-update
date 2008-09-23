#
# Makefile.am for online-update/src
#
rncdir =  $(schemadir)/autoyast/rnc

client_DATA =				\
	online_update_select.ycp 	\
	online_update_install.ycp       \
	online_update.ycp		\
	cd_update.ycp			\
	inst_you.ycp			\
	do_online_update_auto.ycp

module_DATA =				\
	OnlineUpdate.ycp		\
	OnlineUpdateDialogs.ycp		\
	OnlineUpdateCallbacks.ycp

desktop_DATA =				\
	online_update.desktop		\
	cd_update.desktop

EXTRA_DIST = $(client_DATA) $(module_DATA) $(desktop_DATA)

include $(top_srcdir)/Makefile.am.common
