Name:     buteo-mtp-qt5
Version:  0.9.11
Release:  1
Summary:  MTP library
License:  BSD and LGPLv2
URL:      https://github.com/sailfishos/buteo-mtp
Source0: %{name}-%{version}.tar.gz
Source1: %{name}.privileges
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(Qt5Xml)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(Qt5Test)
BuildRequires: pkgconfig(blkid)
BuildRequires: pkgconfig(mount)
BuildRequires: pkgconfig(mlite5)
# for the thumbnailer unit test
BuildRequires: pkgconfig(Qt5Gui)
BuildRequires: pkgconfig(systemsettings) >= 0.6.0
BuildRequires: pkgconfig(nemodbus) >= 2.1.16
Requires: mtp-vendor-configuration
Requires: thumbnaild
Requires(pre): shadow-utils
Requires(pre): /usr/bin/groupadd-user

%description
%{summary}.

%package sample-vendor-configuration
Summary: Vendor configuration example for MTP
Provides: mtp-vendor-configuration

%description sample-vendor-configuration
%{summary}.

%package devel
Summary: Development files for %{name}
Requires: %{name} = %{version}-%{release}

%description devel
%{summary}.

%package tests
Summary: Tests for %{name}
Requires: %{name} = %{version}-%{release}
Conflicts: buteo-mtp-tests

%description tests
%{summary}.

%prep
%setup -q

%build
%qmake5
%make_build

%install
%qmake5_install

mkdir -p %{buildroot}%{_datadir}/mapplauncherd/privileges.d
install -m 644 -p %{SOURCE1} %{buildroot}%{_datadir}/mapplauncherd/privileges.d/

# create group if it does not exist yet, though don't remove it
# as it should come from other packages
%pre
groupadd -f -g 1024 mtp
groupadd-user mtp

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%{_libexecdir}/mtp_service
%{_libdir}/*.so.*
%{_libdir}/mtp
%{_userunitdir}/buteo-mtp.service
%{_datadir}/mapplauncherd/privileges.d/*
# Own the fstorage.d and mtp data directories.
%dir %{_sysconfdir}/fsstorage.d
%dir %{_datadir}/mtp
%license COPYING

# TODO: the deviceinfo xml here should only contain things like model,
#       vendor, ... -- the supported datatypes are tied in tighly with
#       the mtp daemon currently, and therefore can't be changed that
#       easily
%files sample-vendor-configuration
%{_datadir}/mtp/*.xml
%{_datadir}/mtp/*.ico
%config %{_sysconfdir}/fsstorage.d/*

%files devel
%{_includedir}/*
%{_libdir}/*.so

%files tests
/opt/tests/buteo-mtp
