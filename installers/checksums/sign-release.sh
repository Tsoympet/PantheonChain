#!/bin/bash
# Sign release artifacts with GPG

set -e

if [ -z "$GPG_KEY_ID" ]; then
    echo "Error: GPG_KEY_ID environment variable not set"
    echo "Usage: GPG_KEY_ID=your-key-id $0 <file1> <file2> ..."
    exit 1
fi

echo "=== Signing ParthenonChain release artifacts with GPG ==="
echo "GPG Key: $GPG_KEY_ID"

for file in "$@"; do
    if [ -f "$file" ]; then
        echo "Signing: $file"
        gpg --local-user "$GPG_KEY_ID" --detach-sign --armor "$file"
        echo "  Created: ${file}.asc"
    else
        echo "WARNING: File not found: $file"
    fi
done

echo "=== Signing complete ===" 
echo ""
echo "To verify signatures:"
echo "  gpg --verify <file>.asc <file>"
