# Checksums & Signatures

This directory contains scripts for generating and verifying checksums and GPG signatures for ParthenonChain release artifacts.

## Scripts

### generate-checksums.sh

Generates SHA-256 and SHA-512 checksums for all release artifacts.

**Usage:**
```bash
./generate-checksums.sh ../windows/*.exe ../macos/*.dmg ../linux/*.deb ../linux/*.rpm
```

**Output:** `parthenon-1.0.0-checksums.txt`

### verify-checksums.sh

Verifies checksums against release artifacts.

**Usage:**
```bash
./verify-checksums.sh parthenon-1.0.0-checksums.txt
```

### sign-release.sh

Signs release artifacts with GPG.

**Usage:**
```bash
export GPG_KEY_ID="your-key-id"
./sign-release.sh ../windows/*.exe ../macos/*.dmg ../linux/*.deb ../linux/*.rpm
```

**Output:** `.asc` files for each artifact

## GPG Key Setup

1. Generate a GPG key (if you don't have one):
   ```bash
   gpg --full-generate-key
   ```

2. Export your public key:
   ```bash
   gpg --armor --export your-key-id > parthenon-release-key.asc
   ```

3. Publish your public key:
   ```bash
   gpg --keyserver keys.openpgp.org --send-keys your-key-id
   ```

## Verification for Users

Users can verify downloads with:

1. **Import the release key:**
   ```bash
   gpg --import parthenon-release-key.asc
   ```

2. **Verify the signature:**
   ```bash
   gpg --verify parthenon-1.0.0-windows-x64-setup.exe.asc parthenon-1.0.0-windows-x64-setup.exe
   ```

3. **Verify checksums:**
   ```bash
   sha256sum -c parthenon-1.0.0-checksums.txt
   ```

## Best Practices

- Keep your GPG private key secure
- Use a strong passphrase
- Back up your key
- Publish your public key fingerprint on official channels
- Sign all release artifacts
- Include checksums in release notes

## CI/CD Integration

The GitHub Actions workflow automatically:
1. Generates checksums for all artifacts
2. Signs artifacts with GPG (if secrets are configured)
3. Uploads signatures to GitHub Releases

Configure these secrets in your repository:
- `GPG_PRIVATE_KEY` - Your GPG private key (armor format)
- `GPG_KEY_ID` - Your GPG key ID

## Example Output

```
# ParthenonChain 1.0.0 - SHA-256 Checksums
# Generated: 2026-01-12 18:00:00 UTC

abc123...  parthenon-1.0.0-windows-x64-setup.exe
def456...  parthenon-1.0.0-macos.dmg
ghi789...  parthenon_1.0.0_amd64.deb
jkl012...  parthenon-1.0.0-1.el8.x86_64.rpm

# SHA-512 Checksums

xyz789...  parthenon-1.0.0-windows-x64-setup.exe
uvw456...  parthenon-1.0.0-macos.dmg
rst123...  parthenon_1.0.0_amd64.deb
opq890...  parthenon-1.0.0-1.el8.x86_64.rpm
```
