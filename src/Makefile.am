#
# Makefile.am for y2c_online_update/src
#
SUBDIRS = include

etcdir = /etc
admdir = /var/adm/fillup-templates
clientsdir = $(YAST2DIR)/clients
menudir = $(YAST2DIR)/menuentries
modulesdir = $(YAST2DIR)/modules


etc_DATA = suseservers
adm_DATA = sysconfig.you

clients_DATA = online_update_start.ycp	\
		online_update_load.ycp	\
		online_update_details.ycp \
		 online_update_select.ycp \
		 online_update_close.ycp  \
		  online_update.ycp

modules_DATA = OnlineUpdate.ycp

menu_DATA = menuentry_online_update.y2cc	\
		menuentry_cd_update.y2cc

EXTRA_DIST = $(clients_DATA) $(modules_DATA) $(menu_DATA) $(etc_DATA) $(adm_DATA)
