#
# spec file for package y2c_online_update
# 
# Copyright  (c)  2000  SuSE GmbH  Nuernberg, Germany.
#
# please send bugfixes or comments to Gabriele Strattner (gs@suse.de)
#

# neededforbuild  autoconf automake ydoc

Vendor:       SuSE GmbH, Nuernberg, Germany
Distribution: SuSE Linux 6.4 (i386)
Name:         y2c_online_update
Release:      1
Copyright:    (c) 2000 SuSE GmbH
Group:        System Environment/YaST
Requires:	y2t_online_update
BuildArchitectures: noarch

Packager:     Gabriele Strattner (gs@suse.de)

Summary:      YaST2 Modules for installation
Version:      1.0.1
Source0:      y2c_online_update-1.0.1.tar.gz

%description
-

%prep

%setup -n y2c_online_update-1.0.1

%build

./configure
make

%install

make install

%{?suse_check}

%files

%dir /usr/lib/YaST2/clients
%dir /usr/lib/YaST2/menuentries
/usr/lib/YaST2/menuentries/menuentry_online_update.ycp
/usr/lib/YaST2/clients/online_update_step1.ycp
/usr/lib/YaST2/clients/online_update.ycp
