Name: buteo-mtp
Version: 0.0.41
Release: 1
Summary: MTP library
Group: System/Libraries
License: LGPLv2.1
URL: https://github.com/nemomobile/buteo-mtp
Source0: %{name}-%{version}.tar.gz
BuildRequires: pkgconfig(contextsubscriber-1.0)
BuildRequires: pkgconfig(qttracker)
BuildRequires: pkgconfig(synccommon)
BuildRequires: pkgconfig(QtSystemInfo)

# buteo-mtp can use org.freedesktop.thumbnails.Thumbnailer1 to create
# thumbnails on request; at least Windows 8 requires thumbnails to be
# generated on the device.
Requires: tumbler

%description
%{summary}.

%files
%defattr(-,root,root,-)
/usr/lib/systemd/user/buteo-mtp.service
%{_bindir}/buteo-mtp
%{_libdir}/*.so.*
%{_libdir}/mtp/*.so
%{_datadir}/mtp/*.xml
%{_libdir}/mtp/mtp_service
%{_libdir}/mtp/start-mtp.sh


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
%{_libdir}/sync/*.so
%config %{_sysconfdir}/sync/profiles/server/*.xml


%package tests
Summary: Tests for %{name}
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}

%description tests
%{summary}.

%files tests
%defattr(-,root,root,-)
/opt/tests/%{name}


%prep
%setup -q


%build
qmake
# breaks on parallel builds
make


%install
make INSTALL_ROOT=%{buildroot} install
chmod +x %{buildroot}/%{_bindir}/buteo-mtp

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig
