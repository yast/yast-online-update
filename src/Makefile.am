#
# Makefile.am for online_update/src
#

admdir = /var/adm/fillup-templates

adm_DATA = sysconfig.onlineupdate

client_DATA =				\
	online_update_start.ycp		\
	online_update_select.ycp 	\
        online_update_install.ycp       \
	online_update.ycp

module_DATA =			\
	OnlineUpdate.ycp        \
        OnlineUpdateCallbacks.ycp

config_DATA =			\
	online_update.y2cc	\
	cd_update.y2cc

EXTRA_DIST = $(client_DATA) $(module_DATA) $(config_DATA) \
	$(adm_DATA)
