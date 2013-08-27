Name: buteo-mtp-qt5
Version: 0.0.44
Release: 1
Summary: MTP library
Group: System/Libraries
License: LGPLv2.1
URL: https://github.com/nemomobile/buteo-mtp
Source0: %{name}-%{version}.tar.gz
BuildRequires: pkgconfig(contextkit-statefs)
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5Sparql)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(Qt5Xml)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(Qt5Test)
BuildRequires: pkgconfig(buteosyncfw5)
BuildRequires: pkgconfig(Qt5SystemInfo)
Requires: mtp-vendor-configuration
# buteo-mtp can use org.freedesktop.thumbnails.Thumbnailer1 to create
# thumbnails on request; at least Windows 8 requires thumbnails to be
# generated on the device.
Requires: tumbler
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
/usr/lib/systemd/user/buteo-mtp.service
%{_unitdir}/*.mount
%{_unitdir}/local-fs.target.wants/*.mount
%{_bindir}/buteo-mtp
%{_libdir}/*.so.*
%{_libdir}/mtp/*.so
%{_libdir}/mtp/mtp_service
%{_libdir}/mtp/start-mtp.sh


%package sample-vendor-configuration
Summary: Vendor configuration example for MTP
Group: System/Libraries
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


%package devel
Summary: Development files for %{name}
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}

%description devel
%{summary}.

%files devel
%defattr(-,root,root,-)
%{_includedir}/*
%{_libdir}/*.so


%package sync-plugin
Summary: MTP plugin for buteo-sync
Group: System/Libraries

%description sync-plugin
%{summary}.

%files sync-plugin
%defattr(-,root,root,-)
%{_libdir}/buteo-plugins-qt5/*.so
%config %{_sysconfdir}/buteo/profiles/server/*.xml


%package tests
Summary: Tests for %{name}
Group: Development/Libraries
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
qmake -qt=5 -recursive
# breaks on parallel builds
make


%install
make INSTALL_ROOT=%{buildroot} install
chmod +x %{buildroot}/%{_bindir}/buteo-mtp
mkdir -p %{buildroot}/%{_unitdir}/local-fs.target.wants
ln -s ../dev-mtp.mount %{buildroot}/%{_unitdir}/local-fs.target.wants/


# create group if it does not exist yet, though don't remove it
# as it should come from other packages
%pre
groupadd -f -g 1024 mtp
groupadd-user mtp


%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig
