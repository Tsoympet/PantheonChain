#!/bin/bash
# Generate checksums for release artifacts

set -e

echo "=== Generating checksums for ParthenonChain release artifacts ==="

OUTPUT_FILE="parthenon-1.0.0-checksums.txt"

# Remove old checksums
rm -f "$OUTPUT_FILE"

# Generate SHA-256 checksums
echo "# ParthenonChain 1.0.0 - SHA-256 Checksums" > "$OUTPUT_FILE"
echo "# Generated: $(date -u +"%Y-%m-%d %H:%M:%S UTC")" >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

for file in "$@"; do
    if [ -f "$file" ]; then
        echo "Calculating SHA-256 for: $file"
        sha256sum "$file" >> "$OUTPUT_FILE"
    fi
done

echo "" >> "$OUTPUT_FILE"
echo "# SHA-512 Checksums" >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

# Generate SHA-512 checksums
for file in "$@"; do
    if [ -f "$file" ]; then
        echo "Calculating SHA-512 for: $file"
        sha512sum "$file" >> "$OUTPUT_FILE"
    fi
done

echo "=== Checksums saved to: $OUTPUT_FILE ==="
cat "$OUTPUT_FILE"
