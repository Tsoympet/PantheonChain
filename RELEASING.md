# Creating a Release

This guide explains how to create a new release of ParthenonChain.

## Prerequisites

- All tests passing
- Version updated in:
  - `VERSION` file
  - `CMakeLists.txt` (project VERSION)
  - `CHANGELOG.md`
- Release notes prepared
- GPG key for signing (optional but recommended)

## Release Process

### 1. Prepare the Release

```bash
# Update version number
echo "1.0.0" > VERSION

# Update CMakeLists.txt
# Change: project(ParthenonChain VERSION 1.0.0 LANGUAGES CXX C)

# Update CHANGELOG.md with release notes
```

### 2. Create and Push the Tag

```bash
# Create annotated tag
git tag -a v1.0.0 -m "Release 1.0.0"

# Push tag to trigger release workflow
git push origin v1.0.0
```

### 3. Automated Build Process

When you push a tag matching `v*.*.*`, the GitHub Actions workflow will:

1. **Build for all platforms**:
   - Windows (NSIS installer)
   - macOS (DMG)
   - Linux (DEB and RPM packages)

2. **Run tests** on all platforms

3. **Generate checksums** (SHA-256 and SHA-512)

4. **Sign artifacts** (if GPG key is configured in secrets)

5. **Create GitHub Release** (as draft)

6. **Upload all artifacts** to the release

### 4. Manual Release Steps

After the automated workflow completes:

1. Go to [GitHub Releases](https://github.com/Tsoympet/PantheonChain/releases)
2. Find the draft release
3. Review the release notes and artifacts
4. Edit release notes if needed
5. Publish the release

## Building Packages Locally

You can build packages locally for testing:

### Using CPack (All Platforms)

```bash
# Configure and build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel

# Create packages
cpack
```

This will create platform-specific packages:
- **Windows**: NSIS installer and ZIP
- **macOS**: DMG and TGZ
- **Linux**: DEB, RPM, and TGZ

### Platform-Specific Installers

#### Windows Installer

```powershell
cd installers/windows
powershell -ExecutionPolicy Bypass -File build.ps1
```

#### macOS DMG

```bash
cd installers/macos
./build.sh
```

#### Linux DEB

```bash
cd installers/linux
./build-deb.sh
```

#### Linux RPM

```bash
cd installers/linux
./build-rpm.sh
```

## Generating Checksums

```bash
cd installers/checksums
./generate-checksums.sh path/to/installer1.exe path/to/installer2.dmg ...
```

## Signing Releases

```bash
export GPG_KEY_ID="your-key-id"
cd installers/checksums
./sign-release.sh path/to/installer1.exe path/to/installer2.dmg ...
```

## Verifying Releases

Users can verify releases using:

```bash
# Verify GPG signature
gpg --verify parthenon-1.0.0-windows-x64-setup.exe.asc parthenon-1.0.0-windows-x64-setup.exe

# Verify checksums
sha256sum -c parthenon-1.0.0-checksums.txt
```

## GitHub Secrets Required

For automated signing, configure these secrets in GitHub:

- `GPG_PRIVATE_KEY`: Your GPG private key (ASCII armored)
- `GPG_KEY_ID`: Your GPG key ID
- `GPG_PASSPHRASE`: Passphrase for GPG key (if protected)

## Hotfix Releases

For urgent bug fixes:

1. Create hotfix branch: `git checkout -b hotfix/v1.0.1`
2. Fix the issue
3. Update version to 1.0.1
4. Test thoroughly
5. Merge to main
6. Tag: `git tag -a v1.0.1 -m "Hotfix release 1.0.1"`
7. Push tag to trigger release

## Troubleshooting

### Release workflow fails

- Check workflow logs in GitHub Actions
- Verify all dependencies are available
- Ensure version numbers are consistent

### Packages don't install

- Test locally before creating release
- Verify dependencies are correctly specified
- Check package metadata

### Checksums don't match

- Ensure deterministic builds
- Don't modify artifacts after building
- Regenerate checksums if rebuilding

## See Also

- [docs/RELEASES.md](../docs/RELEASES.md) - Detailed release documentation
- [CHANGELOG.md](../CHANGELOG.md) - Version history
- [.github/workflows/release.yml](../.github/workflows/release.yml) - Release workflow
