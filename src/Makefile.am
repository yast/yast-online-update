#
# Makefile.am for y2c_online_update/src
#

etcdir = /etc
admdir = /var/adm/fillup-templates

etc_DATA = suseservers
adm_DATA = sysconfig.onlineupdate

client_DATA =				\
	online_update_start.ycp		\
	online_update_load.ycp		\
	online_update_details.ycp	\
	online_update_select.ycp 	\
	online_update_close.ycp		\
	online_update.ycp

module_DATA =			\
	OnlineUpdate.ycp

config_DATA =			\
	online_update.y2cc	\
	cd_update.y2cc

image_DATA =			\
	online_update_long.png

EXTRA_DIST = $(client_DATA) $(module_DATA) $(config_DATA) $(etc_DATA) $(adm_DATA) $(image_DATA)
