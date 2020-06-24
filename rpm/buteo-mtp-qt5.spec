Name:     buteo-mtp-qt5
Version:  0.7.1
Release:  1
Summary:  MTP library
License:  BSD and LGPLv2
URL:      https://git.sailfishos.org/mer-core/buteo-mtp
Source0: %{name}-%{version}.tar.gz
Source1: %{name}.privileges
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5Sparql)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(Qt5Xml)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(Qt5Test)
BuildRequires: pkgconfig(buteosyncfw5)
BuildRequires: pkgconfig(Qt5SystemInfo)
BuildRequires: pkgconfig(blkid)
BuildRequires: pkgconfig(mount)
BuildRequires: pkgconfig(mlite5)
# for the thumbnailer unit test
BuildRequires: pkgconfig(Qt5Gui)
BuildRequires: ssu-devel >= 0.37.9
BuildRequires: pkgconfig(systemsettings) >= 0.2.25
Requires: mtp-vendor-configuration
Requires: thumbnaild
Requires: libqt5sparql-tracker-direct
Requires: libqt5sparql-tracker
Requires(pre): shadow-utils
Requires(pre): /usr/bin/groupadd-user
Provides: buteo-mtp = %{version}
Obsoletes: buteo-mtp < %{version}

%description
%{summary}.

# TODO: once proper activation as msyncd plugin works as expected,
#       move user session startup into sub-package
%files
%defattr(-,root,root,-)
%{_unitdir}/*.mount
%{_unitdir}/local-fs.target.wants/*.mount
%{_libexecdir}/mtp_service
%{_libdir}/*.so.*
%{_libdir}/mtp
%{_userunitdir}/buteo-mtp.service
%{_datadir}/mapplauncherd/privileges.d/*
# Own the fstorage.d and mtp data directories.
%dir %{_sysconfdir}/fsstorage.d
%dir %{_datadir}/mtp
%license COPYING

%package sample-vendor-configuration
Summary: Vendor configuration example for MTP
Provides: mtp-vendor-configuration

%description sample-vendor-configuration
%{summary}.

# TODO: the deviceinfo xml here should only contain things like model,
#       vendor, ... -- the supported datatypes are tied in tighly with
#       the mtp daemon currently, and therefore can't be changed that
#       easily
%files sample-vendor-configuration
%defattr(-,root,root,-)
%{_datadir}/mtp/*.xml
%{_datadir}/mtp/*.ico
%config %{_sysconfdir}/fsstorage.d/*

%package devel
Summary: Development files for %{name}
Requires: %{name} = %{version}-%{release}

%description devel
%{summary}.

%files devel
%defattr(-,root,root,-)
%{_includedir}/*
%{_libdir}/*.so


%package sync-plugin
Summary: MTP plugin for buteo-sync

%description sync-plugin
%{summary}.

%files sync-plugin
%defattr(-,root,root,-)
%{_libdir}/buteo-plugins-qt5/*.so
%config %{_sysconfdir}/buteo/profiles/server/*.xml


%package tests
Summary: Tests for %{name}
Requires: %{name} = %{version}-%{release}
Conflicts: buteo-mtp-tests

%description tests
%{summary}.

%files tests
%defattr(-,root,root,-)
/opt/tests/buteo-mtp


%prep
%setup -q


%build
%qmake5
make %{_smp_mflags}


%install
make INSTALL_ROOT=%{buildroot} install
mkdir -p %{buildroot}/%{_unitdir}/local-fs.target.wants
ln -s ../dev-mtp.mount %{buildroot}/%{_unitdir}/local-fs.target.wants/

mkdir -p %{buildroot}%{_datadir}/mapplauncherd/privileges.d
install -m 644 -p %{SOURCE1} %{buildroot}%{_datadir}/mapplauncherd/privileges.d/

# create group if it does not exist yet, though don't remove it
# as it should come from other packages
%pre
groupadd -f -g 1024 mtp
groupadd-user mtp


%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig
