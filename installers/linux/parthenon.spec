Name:           pantheonchain
Version:        2.3.0
Release:        1%{?dist}
Summary:        PantheonChain — three-layer PoW/PoS hybrid blockchain with EVM

License:        MIT
URL:            https://pantheonchain.org
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.15
BuildRequires:  gcc-c++
BuildRequires:  openssl-devel
BuildRequires:  boost-devel

Requires:       openssl >= 3.0.0
Requires:       boost-system
Requires:       boost-filesystem
Requires:       boost-thread
Recommends:     curl

%description
PantheonChain is a production-grade three-layer blockchain:
- TALANTON (L1): SHA-256d Proof-of-Work settlement layer (21M TALN hard cap)
- DRACHMA  (L2): PoS/BFT payments and validator layer  (41M DRM hard cap)
- OBOLOS   (L3): EVM-compatible smart-contract execution (61M OBL hard cap)

Key features: Schnorr BIP-340 signatures, cross-layer commitment anchoring,
1A1V governance, Prometheus metrics, hardware wallet support (Ledger/Trezor).

%prep
%setup -q

%build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/etc/pantheon/mainnet
mkdir -p $RPM_BUILD_ROOT/usr/lib/systemd/system
mkdir -p $RPM_BUILD_ROOT/usr/share/doc/%{name}
mkdir -p $RPM_BUILD_ROOT/var/lib/pantheon/{l1,l2,l3}

# Install node binaries
install -m 755 build/pantheon-l1-talanton-node $RPM_BUILD_ROOT/usr/bin/
install -m 755 build/pantheon-l2-drachma-node  $RPM_BUILD_ROOT/usr/bin/
install -m 755 build/pantheon-l3-obolos-node   $RPM_BUILD_ROOT/usr/bin/
install -m 755 build/pantheon-cli              $RPM_BUILD_ROOT/usr/bin/
install -m 755 build/pantheon-relayer-l2       $RPM_BUILD_ROOT/usr/bin/
install -m 755 build/pantheon-relayer-l3       $RPM_BUILD_ROOT/usr/bin/

# Install configuration
install -m 644 configs/mainnet/l1.conf $RPM_BUILD_ROOT/etc/pantheon/mainnet/
install -m 644 configs/mainnet/l2.conf $RPM_BUILD_ROOT/etc/pantheon/mainnet/
install -m 644 configs/mainnet/l3.conf $RPM_BUILD_ROOT/etc/pantheon/mainnet/
install -m 644 configs/mainnet/l1.json $RPM_BUILD_ROOT/etc/pantheon/mainnet/
install -m 644 configs/mainnet/l2.json $RPM_BUILD_ROOT/etc/pantheon/mainnet/
install -m 644 configs/mainnet/l3.json $RPM_BUILD_ROOT/etc/pantheon/mainnet/
install -m 644 genesis_talanton.json   $RPM_BUILD_ROOT/etc/pantheon/
install -m 644 genesis_drachma.json    $RPM_BUILD_ROOT/etc/pantheon/
install -m 644 genesis_obolos.json     $RPM_BUILD_ROOT/etc/pantheon/

# Install systemd units
install -m 644 installers/linux/systemd/pantheon-l1.service       $RPM_BUILD_ROOT/usr/lib/systemd/system/
install -m 644 installers/linux/systemd/pantheon-l2.service       $RPM_BUILD_ROOT/usr/lib/systemd/system/
install -m 644 installers/linux/systemd/pantheon-l3.service       $RPM_BUILD_ROOT/usr/lib/systemd/system/
install -m 644 installers/linux/systemd/pantheon-relayer-l2.service $RPM_BUILD_ROOT/usr/lib/systemd/system/
install -m 644 installers/linux/systemd/pantheon-relayer-l3.service $RPM_BUILD_ROOT/usr/lib/systemd/system/

# Install documentation
install -m 644 README.md    $RPM_BUILD_ROOT/usr/share/doc/%{name}/
install -m 644 LICENSE      $RPM_BUILD_ROOT/usr/share/doc/%{name}/
install -m 644 CHANGELOG.md $RPM_BUILD_ROOT/usr/share/doc/%{name}/
install -m 644 WHITEPAPER.md $RPM_BUILD_ROOT/usr/share/doc/%{name}/

%pre
getent group pantheon >/dev/null || groupadd -r pantheon
getent passwd pantheon >/dev/null || \
    useradd -r -g pantheon -d /var/lib/pantheon -s /sbin/nologin \
    -c "PantheonChain daemon" pantheon
exit 0

%post
%systemd_post pantheon-l1.service pantheon-l2.service pantheon-l3.service
%systemd_post pantheon-relayer-l2.service pantheon-relayer-l3.service

%preun
%systemd_preun pantheon-l1.service pantheon-l2.service pantheon-l3.service
%systemd_preun pantheon-relayer-l2.service pantheon-relayer-l3.service

%postun
%systemd_postun_with_restart pantheon-l1.service pantheon-l2.service pantheon-l3.service
if [ $1 -eq 0 ]; then
    userdel pantheon 2>/dev/null || :
fi

%files
%defattr(-,root,root,-)
%{_bindir}/pantheon-l1-talanton-node
%{_bindir}/pantheon-l2-drachma-node
%{_bindir}/pantheon-l3-obolos-node
%{_bindir}/pantheon-cli
%{_bindir}/pantheon-relayer-l2
%{_bindir}/pantheon-relayer-l3
%config(noreplace) %{_sysconfdir}/pantheon/mainnet/l1.conf
%config(noreplace) %{_sysconfdir}/pantheon/mainnet/l2.conf
%config(noreplace) %{_sysconfdir}/pantheon/mainnet/l3.conf
%config(noreplace) %{_sysconfdir}/pantheon/mainnet/l1.json
%config(noreplace) %{_sysconfdir}/pantheon/mainnet/l2.json
%config(noreplace) %{_sysconfdir}/pantheon/mainnet/l3.json
%{_sysconfdir}/pantheon/genesis_talanton.json
%{_sysconfdir}/pantheon/genesis_drachma.json
%{_sysconfdir}/pantheon/genesis_obolos.json
%{_unitdir}/pantheon-l1.service
%{_unitdir}/pantheon-l2.service
%{_unitdir}/pantheon-l3.service
%{_unitdir}/pantheon-relayer-l2.service
%{_unitdir}/pantheon-relayer-l3.service
%doc %{_docdir}/%{name}/README.md
%doc %{_docdir}/%{name}/LICENSE
%doc %{_docdir}/%{name}/CHANGELOG.md
%doc %{_docdir}/%{name}/WHITEPAPER.md
%dir %attr(750,pantheon,pantheon) /var/lib/pantheon
%dir %attr(750,pantheon,pantheon) /var/lib/pantheon/l1
%dir %attr(750,pantheon,pantheon) /var/lib/pantheon/l2
%dir %attr(750,pantheon,pantheon) /var/lib/pantheon/l3

%changelog
* Mon Mar 09 2026 PantheonChain Foundation <contact@pantheonchain.org> - 2.3.0-1
- Production readiness: bootstrap nodes, Prometheus metrics, systemd units
- CMakeLists reads version from VERSION.txt; install() and CPack rules added
- Package name pantheonchain (was parthenon), version 2.3.0
- Five systemd service files with security hardening
- docker-compose.testnet.yml with resource limits and structured logging
- Expanded CLI: wallet, account, staking, governance, node commands
- Improved SECURITY.md with severity matrix and SLA table
- Added mainnet_launch_checklist.md, validator_runbook.md
- Added setup-validator.sh wizard and run-mainnet.sh production launcher

