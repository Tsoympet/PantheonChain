#!/bin/bash
# Verify checksums for ParthenonChain release artifacts

set -e

if [ $# -ne 1 ]; then
    echo "Usage: $0 <checksums-file>"
    exit 1
fi

CHECKSUMS_FILE="$1"

if [ ! -f "$CHECKSUMS_FILE" ]; then
    echo "Error: Checksums file not found: $CHECKSUMS_FILE"
    exit 1
fi

echo "=== Verifying checksums from: $CHECKSUMS_FILE ==="

# Extract SHA-256 section
SHA256_SECTION=$(awk '/^# SHA-256/,/^# SHA-512/' "$CHECKSUMS_FILE" | grep -v "^#" | grep -v "^$")

# Verify SHA-256
echo "$SHA256_SECTION" | while read -r hash file; do
    if [ -f "$file" ]; then
        echo -n "Verifying SHA-256: $file ... "
        calculated=$(sha256sum "$file" | awk '{print $1}')
        if [ "$hash" = "$calculated" ]; then
            echo "OK"
        else
            echo "FAILED"
            echo "  Expected: $hash"
            echo "  Got:      $calculated"
            exit 1
        fi
    else
        echo "WARNING: File not found: $file"
    fi
done

# Extract SHA-512 section
SHA512_SECTION=$(awk '/^# SHA-512/,EOF' "$CHECKSUMS_FILE" | grep -v "^#" | grep -v "^$")

# Verify SHA-512
echo "$SHA512_SECTION" | while read -r hash file; do
    if [ -f "$file" ]; then
        echo -n "Verifying SHA-512: $file ... "
        calculated=$(sha512sum "$file" | awk '{print $1}')
        if [ "$hash" = "$calculated" ]; then
            echo "OK"
        else
            echo "FAILED"
            echo "  Expected: $hash"
            echo "  Got:      $calculated"
            exit 1
        fi
    else
        echo "WARNING: File not found: $file"
    fi
done

echo "=== All checksums verified successfully ==="
