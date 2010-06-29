Name: buteo-mtp
Version: 0.0.36
Release: 1
Summary: MTP library
Group: System/Libraries
License: LGPLv2.1
URL: http://meego.gitorious.com/meego-middleware/buteo-mtp
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: pkgconfig(contextsubscriber-1.0)
BuildRequires: buteo-syncfw-devel
BuildRequires: libqttracker-devel

%description
%{summary}.

%files
%defattr(-,root,root,-)
%config %{_sysconfdir}/sync
%{_libdir}/*.so.*
%{_libdir}/sync/*.so
%{_libdir}/mtp/*.so
%{_datadir}/mtp/*.xml


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


%package tests
Summary: Tests for %{name}
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}

%description tests
%{summary}.

%files tests
%defattr(-,root,root,-)
%{_datadir}/libmeegomtp-tests
%{_bindir}/*test


%prep
%setup -q


%build
qmake
# breaks on parallel builds
make


%install
rm -rf %{buildroot}

make INSTALL_ROOT=%{buildroot} install


%clean
rm -rf %{buildroot}


%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig
