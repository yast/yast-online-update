#
# Makefile.am for y2c_online_update/src
#
SUBDIRS = include

modulesdir = $(YAST2DIR)/clients
menudir = $(YAST2DIR)/menuentries
updatedir = $(YAST2DIR)/update

modules_DATA = online_update_start.ycp	\
		online_update_load.ycp	\
		online_update_details.ycp \
		 online_update_select.ycp \
		 online_update_close.ycp  \
		  online_update.ycp

update_DATA = ftp_server.ycp

menu_DATA = menuentry_online_update.ycp

EXTRA_DIST = $(modules_DATA) $(menu_DATA) $(update_DATA)
