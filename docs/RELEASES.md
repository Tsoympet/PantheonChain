# Release Process

This document describes how ParthenonChain releases are created, tested, and distributed.

## Release Cycle

**Versioning**: Semantic Versioning (MAJOR.MINOR.PATCH)

- **MAJOR**: Consensus-breaking changes (hard forks)
- **MINOR**: New features, non-consensus changes
- **PATCH**: Bug fixes, security patches

**Release Schedule**:
- Major releases: As needed for hard forks
- Minor releases: Quarterly
- Patch releases: As needed for critical bugs

## Version Numbers

**Current Version**: 1.0.0

**Upcoming**:
- 1.0.x - Patch releases for bug fixes
- 1.1.0 - Next feature release
- 2.0.0 - Future hard fork (if needed)

## Release Checklist

### 1. Pre-Release

- [ ] All planned features complete
- [ ] All tests passing
- [ ] Code review complete
- [ ] Security audit (for major releases)
- [ ] Documentation updated
- [ ] CHANGELOG.md updated
- [ ] VERSION file updated
- [ ] No known critical bugs

### 2. Testing

- [ ] Unit tests: 100% pass
- [ ] Integration tests: 100% pass
- [ ] Consensus tests: 100% pass
- [ ] Regression tests: 100% pass
- [ ] Manual testing on all platforms
- [ ] Testnet deployment and validation

### 3. Build Preparation

- [ ] Update version numbers in all files
- [ ] Create release branch: `release/v1.0.0`
- [ ] Tag release: `git tag -a v1.0.0 -m "Release 1.0.0"`
- [ ] Build all platform installers
- [ ] Verify installer integrity

### 4. Quality Assurance

- [ ] Install and test on clean Windows system
- [ ] Install and test on clean macOS system
- [ ] Install and test on clean Linux system
- [ ] Verify upgrade path from previous version
- [ ] Test all major features
- [ ] Performance benchmarks

### 5. Security

- [ ] Generate checksums (SHA-256, SHA-512)
- [ ] GPG sign all artifacts
- [ ] Verify signatures
- [ ] Scan for vulnerabilities
- [ ] Review dependencies

### 6. Documentation

- [ ] Release notes written
- [ ] Migration guide (if needed)
- [ ] API changes documented
- [ ] Known issues listed
- [ ] Installation instructions verified

### 7. Distribution

- [ ] Create GitHub Release (draft)
- [ ] Upload all installers
- [ ] Upload checksums and signatures
- [ ] Add release notes
- [ ] Publish release
- [ ] Update website (if applicable)

### 8. Announcement

- [ ] Announce on GitHub
- [ ] Social media posts
- [ ] Email notification (if mailing list exists)
- [ ] Update documentation site
- [ ] Notify exchanges and services

### 9. Post-Release

- [ ] Monitor for issues
- [ ] Respond to bug reports
- [ ] Prepare patch release if needed
- [ ] Update roadmap
- [ ] Begin next release cycle

## Automated Release Workflow

Releases are automated via GitHub Actions (`.github/workflows/release.yml`).

### Trigger

**Option 1: Tag Push (Recommended)**

Push a version tag:
```bash
git tag -a v1.0.0 -m "Release 1.0.0"
git push origin v1.0.0
```

**Option 2: Manual Workflow Dispatch**

Trigger the workflow manually from GitHub Actions UI:

1. Go to GitHub Actions tab
2. Select "Release Build" workflow
3. Click "Run workflow"
4. Specify:
   - **Version**: Version number without 'v' prefix (e.g., `1.0.0`) - **Required**
   - **Draft**: Whether to create as draft release (default: true)

Alternatively, use GitHub CLI:
```bash
# Run with version (required) and draft option
# Note: Provide version WITHOUT 'v' prefix (e.g., 1.0.0, not v1.0.0)
gh workflow run release.yml -f version=1.0.0 -f draft=true

# Run with version to create non-draft release
gh workflow run release.yml -f version=1.0.0 -f draft=false
```

### Workflow Steps

1. **Checkout**: Fetch repository and submodules
2. **Build Matrix**: Parallel builds on Windows, macOS, Linux
3. **Compile**: Build binaries for each platform
4. **Test**: Run test suite on each platform
5. **Package**: Create installers (NSIS, DMG, DEB, RPM)
6. **Checksums**: Generate SHA-256 and SHA-512 checksums
7. **Sign**: GPG sign all artifacts (if secrets configured)
8. **Upload**: Upload artifacts to GitHub Actions
9. **Release**: Create GitHub Release (draft mode)
10. **Notify**: Send notifications (if configured)

### Build Platforms

| Platform | OS | Arch | Installer |
|----------|-------|------|-----------|
| Windows | windows-latest | x64 | .exe (NSIS) |
| macOS | macos-latest | universal | .dmg |
| Linux (Debian) | ubuntu-latest | amd64 | .deb |
| Linux (RPM) | ubuntu-latest | x86_64 | .rpm |

## Manual Build Process

For manual releases or custom builds:

### Windows Installer

```powershell
# Build binaries
mkdir build && cd build
cmake -G "Visual Studio 16 2019" -A x64 ..
cmake --build . --config Release

# Create installer
cd ../installers/windows
makensis parthenon-installer.nsi
```

Output: `parthenon-1.0.0-windows-x64-setup.exe`

### macOS DMG

```bash
# Build binaries
mkdir build && cd build
cmake -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" ..
make -j$(sysctl -n hw.ncpu)

# Create DMG
cd ../installers/macos
./create-dmg.sh
```

Output: `parthenon-1.0.0-macos.dmg`

### Linux DEB

```bash
# Build binaries
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Create package
cd ../installers/linux
./build-deb.sh
```

Output: `parthenon_1.0.0_amd64.deb`

### Linux RPM

```bash
# Build binaries
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Create package
cd ../installers/linux
rpmbuild -ba parthenon.spec
```

Output: `parthenon-1.0.0-1.el8.x86_64.rpm`

## Checksums and Signatures

### Generate Checksums

```bash
cd installers/checksums
./generate-checksums.sh \
  ../windows/*.exe \
  ../macos/*.dmg \
  ../linux/*.deb \
  ../linux/*.rpm
```

Output: `parthenon-1.0.0-checksums.txt`

### Sign Artifacts

```bash
export GPG_KEY_ID="your-key-id"
cd installers/checksums
./sign-release.sh \
  ../windows/*.exe \
  ../macos/*.dmg \
  ../linux/*.deb \
  ../linux/*.rpm
```

Output: `.asc` files for each artifact

## GitHub Secrets

Required secrets for automated signing:

- `GPG_PRIVATE_KEY`: GPG private key (armor format)
- `GPG_KEY_ID`: GPG key ID
- `GPG_PASSPHRASE`: GPG key passphrase (if protected)

## Release Notes Template

```markdown
# ParthenonChain v1.0.0

Release Date: 2026-01-12

## Overview

Brief description of this release.

## New Features

- Feature 1
- Feature 2

## Improvements

- Improvement 1
- Improvement 2

## Bug Fixes

- Fix #123: Description
- Fix #456: Description

## Security

- Security improvement 1

## Breaking Changes

- None / List breaking changes

## Upgrading

Instructions for upgrading from previous version.

## Known Issues

- Issue 1
- Issue 2

## Checksums

SHA-256:
```
abc123... parthenon-1.0.0-windows-x64-setup.exe
def456... parthenon-1.0.0-macos.dmg
ghi789... parthenon_1.0.0_amd64.deb
jkl012... parthenon-1.0.0-1.el8.x86_64.rpm
```

## Downloads

- Windows: [parthenon-1.0.0-windows-x64-setup.exe](url)
- macOS: [parthenon-1.0.0-macos.dmg](url)
- Linux (Debian): [parthenon_1.0.0_amd64.deb](url)
- Linux (RPM): [parthenon-1.0.0-1.el8.x86_64.rpm](url)
- Source: [Source code (tar.gz)](url)

## Verification

See [INSTALLATION.md](INSTALLATION.md#verification) for verification instructions.
```

## Hotfix Process

For critical security issues or bugs:

1. Create hotfix branch: `hotfix/v1.0.1`
2. Fix the issue
3. Test thoroughly
4. Update VERSION and CHANGELOG
5. Tag: `v1.0.1`
6. Build and release
7. Announce urgently
8. Merge back to main and develop

## Hard Fork Activation

For consensus-breaking changes:

1. **BIP Process**: Document the change
2. **Community Discussion**: Gather feedback
3. **Implementation**: Code the change with activation logic
4. **Testing**: Extensive testnet deployment
5. **Activation Height**: Choose block height far in future
6. **Release**: Multiple releases before activation
7. **Monitoring**: Watch for activation progress
8. **Post-Activation**: Monitor for issues

Example activation:

```cpp
const uint32_t FORK_ACTIVATION_HEIGHT = 500000;

bool IsForked(uint32_t height) {
    return height >= FORK_ACTIVATION_HEIGHT;
}
```

## Deprecation Policy

Features to be removed:

1. Mark as deprecated in documentation
2. Add deprecation warnings in code
3. Wait minimum 2 major versions
4. Remove in future release
5. Document in CHANGELOG

## Support Policy

| Version | Status | Support Until |
|---------|--------|---------------|
| 1.0.x | Current | Ongoing |
| 0.x.x | Unsupported | N/A |

**Support includes**:
- Security patches
- Critical bug fixes
- Minor improvements

**End of support**:
- Announced 6 months in advance
- Final patch release before EOL

## Reproducible Builds

To verify builds are deterministic:

```bash
# Build 1
git clone https://github.com/Tsoympet/PantheonChain.git build1
cd build1
git checkout v1.0.0
mkdir build && cd build
cmake .. && make

# Build 2
git clone https://github.com/Tsoympet/PantheonChain.git build2
cd build2
git checkout v1.0.0
mkdir build && cd build
cmake .. && make

# Compare
diff -r ../build1/build ../build2/build
```

Binaries should be byte-for-byte identical.

---

**See Also**:
- [CHANGELOG.md](../CHANGELOG.md)
- [INSTALLATION.md](INSTALLATION.md)
- [Phase 10 Completion Report](PHASE10_COMPLETION.md)
