#
# Makefile.am for y2c_online_update/src
#
SUBDIRS = include

etcdir = /etc
admdir = /var/adm/fillup-templates
modulesdir = $(YAST2DIR)/clients
menudir = $(YAST2DIR)/menuentries

etc_DATA = suseservers
adm_DATA = rc.config.y2c_online_update

modules_DATA = online_update_start.ycp	\
		online_update_load.ycp	\
		online_update_details.ycp \
		 online_update_select.ycp \
		 online_update_close.ycp  \
		  online_update.ycp

menu_DATA = menuentry_online_update.ycp

EXTRA_DIST = $(modules_DATA) $(menu_DATA) $(etc_DATA) $(adm_DATA)
