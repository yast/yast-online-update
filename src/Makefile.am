#
# Makefile.am for online-update/src
#

ynclude_DATA = online_update_dialogs.ycp

client_DATA =				\
	online_update_start.ycp		\
	online_update_auto.ycp		\
	online_update_select.ycp 	\
	online_update_install.ycp       \
	online_update.ycp		\
        online_update_dialogs.ycp       \
	inst_you.ycp

module_DATA =				\
	OnlineUpdate.ycp		\
	AutoOnlineUpdate.ycp		\
	OnlineUpdateCallbacks.ycp

desktop_DATA =				\
	online_update.desktop		\
	cd_update.desktop

EXTRA_DIST = $(client_DATA) $(module_DATA) $(desktop_DATA)

include $(top_srcdir)/Makefile.am.common
