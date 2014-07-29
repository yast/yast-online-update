#
# spec file for package yast2-online-update
#
# Copyright (c) 2013 SUSE LINUX Products GmbH, Nuernberg, Germany.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/
#


Name:           yast2-online-update
Version:        3.1.6
Release:        0
Url:            https://github.com/yast/yast-online-update

BuildRoot:      %{_tmppath}/%{name}-%{version}-build
Source0:        %{name}-%{version}.tar.bz2

Group:          System/YaST
License:        GPL-2.0
BuildRequires:	gcc-c++ libtool update-desktop-files yast2-packager 
BuildRequires:  yast2-devtools >= 3.1.10
BuildRequires:  rubygem-rspec
# Product EOL tag
Requires:	yast2-pkg-bindings >= 3.1.6
# Kernel::InformAboutKernelChange
Requires:	yast2 >= 2.23.8
# PackageCallbacks::FormatPatchName
Requires:	yast2-packager >= 2.13.159

Provides:	y2c_online_update yast2-config-online-update
Obsoletes:	y2c_online_update yast2-config-online-update
Provides:	yast2-trans-online-update y2t_online_update
Obsoletes:	yast2-trans-online-update y2t_online_update
BuildArchitectures:     noarch

# Added Logger (replacement for y2error, y2milestone, ...)
Requires:       yast2-ruby-bindings >= 3.1.7

Summary:    	YaST2 - Online Update (YOU)

%description
YaST Online Update (YOU) provides a convenient way to download and
install security and other system updates. By default it uses the
official SUSE mirrors as the update sources, but it can also use local
patch repositories or patch CDs.

This package provides the graphical user interface for YOU which can be
used with or without the X Window System. It can be started from the
YaST control center.

%prep
%setup -n %{name}-%{version}

%build
%yast_build

%install
%yast_install


%files
%defattr(-,root,root)
%{yast_scrconfdir}/*.scr
%{yast_clientdir}/online*.rb
%{yast_clientdir}/cd_update.rb
%{yast_clientdir}/inst_you.rb
%{yast_clientdir}/do_online_update_auto.rb
%{yast_moduledir}/OnlineUpdate*.rb
%doc %{yast_docdir}

%package frontend
Summary:	YaST2 - Online Update (YOU)
Requires:	yast2-online-update
Group:          System/YaST

# PatchCD desktop file moved to yast2-wagon
Conflicts:	yast2-wagon <= 2.17.3

%description frontend
Desktop files for YaST2 online update

%files frontend
%defattr(-,root,root)
%{yast_desktopdir}/online_update.desktop
