#!/bin/bash
# Generate checksums for release artifacts

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Read version from VERSION file
VERSION=$(cat "$PROJECT_ROOT/VERSION" 2>/dev/null || echo "1.0.0")

echo "=== Generating checksums for ParthenonChain release artifacts ==="

OUTPUT_FILE="parthenon-${VERSION}-checksums.txt"

# Remove old checksums
rm -f "$OUTPUT_FILE"

# Generate SHA-256 checksums
{
    echo "# ParthenonChain ${VERSION} - SHA-256 Checksums"
    echo "# Generated: $(date -u +"%Y-%m-%d %H:%M:%S UTC")"
    echo ""
} > "$OUTPUT_FILE"

for file in "$@"; do
    if [ -f "$file" ]; then
        echo "Calculating SHA-256 for: $file"
        sha256sum "$file" >> "$OUTPUT_FILE"
    fi
done

{
    echo ""
    echo "# SHA-512 Checksums"
    echo ""
} >> "$OUTPUT_FILE"

# Generate SHA-512 checksums
for file in "$@"; do
    if [ -f "$file" ]; then
        echo "Calculating SHA-512 for: $file"
        sha512sum "$file" >> "$OUTPUT_FILE"
    fi
done

echo "=== Checksums saved to: $OUTPUT_FILE ==="
cat "$OUTPUT_FILE"
