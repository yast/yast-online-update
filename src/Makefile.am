#
# Makefile.am for y2c_online_update/src
#
SUBDIRS = include

etcdir = /etc
admdir = /var/adm/fillup-templates
clientsdir = $(YAST2DIR)/clients
menudir = $(YAST2DIR)/menuentries
modulesdir = $(YAST2DIR)/modules
imagedir = $(YAST2DIR)/images

etc_DATA = suseservers
adm_DATA = sysconfig.onlineupdate

clients_DATA = online_update_start.ycp	\
		online_update_load.ycp	\
		online_update_details.ycp \
		 online_update_select.ycp \
		 online_update_close.ycp  \
		  online_update.ycp

modules_DATA = OnlineUpdate.ycp

menu_DATA = menuentry_online_update.y2cc	\
		menuentry_cd_update.y2cc

image_DATA = online_update_long.png

EXTRA_DIST = $(clients_DATA) $(modules_DATA) $(menu_DATA) $(etc_DATA) $(adm_DATA) $(image_DATA)
