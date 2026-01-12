Name:           parthenon
Version:        1.0.0
Release:        1%{?dist}
Summary:        ParthenonChain - Multi-Asset Proof-of-Work Blockchain

License:        MIT
URL:            https://parthenonchain.org
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.15
BuildRequires:  gcc-c++
BuildRequires:  openssl-devel
BuildRequires:  boost-devel

Requires:       openssl >= 3.0.0
Requires:       boost-system
Requires:       boost-filesystem
Requires:       boost-thread

%description
ParthenonChain is a production-grade Layer-1 blockchain featuring:
- Three native assets (TALN, DRM, OBL) with deterministic issuance
- SHA-256d Proof-of-Work consensus with timewarp protection
- EVM-compatible smart contracts powered by OBL gas
- DRM settlement primitives (escrow, multisig, destination tags)
- Layer 2 payment channels and HTLC support
- Full node daemon, CLI tools, and GUI wallet

%prep
%setup -q

%build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/etc/parthenon
mkdir -p $RPM_BUILD_ROOT/usr/lib/systemd/system
mkdir -p $RPM_BUILD_ROOT/usr/share/doc/%{name}
mkdir -p $RPM_BUILD_ROOT/var/lib/parthenon

# Install binaries
install -m 755 build/clients/core-daemon/parthenond $RPM_BUILD_ROOT/usr/bin/
install -m 755 build/clients/cli/parthenon-cli $RPM_BUILD_ROOT/usr/bin/
install -m 755 build/clients/desktop/parthenon-qt $RPM_BUILD_ROOT/usr/bin/

# Install configuration
install -m 644 build/clients/core-daemon/parthenond.conf $RPM_BUILD_ROOT/etc/parthenon/

# Install systemd service
cat > $RPM_BUILD_ROOT/usr/lib/systemd/system/parthenond.service << EOF
[Unit]
Description=ParthenonChain Daemon
After=network.target

[Service]
Type=simple
User=parthenon
Group=parthenon
ExecStart=/usr/bin/parthenond
Restart=on-failure
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

# Install documentation
install -m 644 README.md $RPM_BUILD_ROOT/usr/share/doc/%{name}/
install -m 644 LICENSE $RPM_BUILD_ROOT/usr/share/doc/%{name}/
install -m 644 CHANGELOG.md $RPM_BUILD_ROOT/usr/share/doc/%{name}/

%pre
getent group parthenon >/dev/null || groupadd -r parthenon
getent passwd parthenon >/dev/null || \
    useradd -r -g parthenon -d /var/lib/parthenon -s /sbin/nologin \
    -c "ParthenonChain daemon" parthenon
exit 0

%post
%systemd_post parthenond.service

%preun
%systemd_preun parthenond.service

%postun
%systemd_postun_with_restart parthenond.service
if [ $1 -eq 0 ]; then
    userdel parthenon 2>/dev/null || :
fi

%files
%defattr(-,root,root,-)
%{_bindir}/parthenond
%{_bindir}/parthenon-cli
%{_bindir}/parthenon-qt
%config(noreplace) %{_sysconfdir}/parthenon/parthenond.conf
%{_unitdir}/parthenond.service
%doc %{_docdir}/%{name}/README.md
%doc %{_docdir}/%{name}/LICENSE
%doc %{_docdir}/%{name}/CHANGELOG.md
%dir %attr(750,parthenon,parthenon) /var/lib/parthenon

%changelog
* $(date +"%a %b %d %Y") ParthenonChain Foundation <dev@parthenonchain.org> - 1.0.0-1
- Initial release
- Full consensus implementation with three native assets
- EVM-compatible smart contracts (OBOLOS)
- Layer 2 payment channels and HTLC support
- Complete client suite (daemon, CLI, GUI)
