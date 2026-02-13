#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT_DIR"

if [[ ! -f .gitmodules ]]; then
  echo "ERROR: .gitmodules is missing; real dependency pinning cannot be verified."
  exit 1
fi

required_submodules=(
  "third_party/secp256k1"
  "third_party/leveldb"
  "third_party/cpp-httplib"
  "third_party/json"
)

for path in "${required_submodules[@]}"; do
  if ! git config -f .gitmodules --get-regexp "^submodule\..*\.path$" | awk '{print $2}' | grep -qx "$path"; then
    echo "ERROR: required submodule '$path' is not defined in .gitmodules"
    exit 1
  fi

done

# Ensure submodules are initialized and checked out to the exact gitlink commit.
while IFS= read -r line; do
  status="${line:0:1}"
  case "$status" in
    " ") ;; # exact gitlink commit
    "-")
      echo "ERROR: uninitialized submodule detected: $line"
      exit 1
      ;;
    "+")
      echo "ERROR: submodule checked out at non-pinned commit: $line"
      exit 1
      ;;
    "U")
      echo "ERROR: submodule has merge conflict: $line"
      exit 1
      ;;
    *)
      echo "ERROR: unknown submodule status '$status' in line: $line"
      exit 1
      ;;
  esac
done < <(git submodule status --recursive)

# Ensure each required submodule has a commit in the index (gitlink entry).
for path in "${required_submodules[@]}"; do
  mode_type_sha_path="$(git ls-files -s "$path" || true)"
  if [[ -z "$mode_type_sha_path" ]]; then
    echo "ERROR: submodule path '$path' not tracked in git index"
    exit 1
  fi

  mode="$(awk '{print $1}' <<<"$mode_type_sha_path")"
  if [[ "$mode" != "160000" ]]; then
    echo "ERROR: '$path' is not a gitlink (expected mode 160000, got $mode)"
    exit 1
  fi
done

archive_manifest="scripts/ci/vendored_archives.sha256"
archive_candidates=()
while IFS= read -r -d '' tracked_file; do
  case "$tracked_file" in
    *.tar.gz|*.tgz|*.zip|*.tar.xz|*.tar.bz2)
      archive_candidates+=("$tracked_file")
      ;;
  esac
done < <(git ls-files -z)

if (( ${#archive_candidates[@]} > 0 )); then
  if [[ ! -f "$archive_manifest" ]]; then
    echo "ERROR: vendored archive(s) detected but checksum manifest '$archive_manifest' is missing."
    echo "Detected archives:"
    printf ' - %s\n' "${archive_candidates[@]}"
    echo "Add sha256 checksums for each archive before continuing."
    exit 1
  fi

  invalid_entries=()
  manifest_paths=()
  while IFS= read -r line; do
    [[ -z "$line" || "$line" =~ ^[[:space:]]*# ]] && continue
    if [[ "$line" =~ ^[0-9a-fA-F]{64}[[:space:]]+([^[:space:]].*)$ ]]; then
      manifest_paths+=("${BASH_REMATCH[1]}")
    else
      invalid_entries+=("$line")
    fi
  done < "$archive_manifest"

  if (( ${#invalid_entries[@]} > 0 )); then
    echo "ERROR: checksum manifest contains invalid entries:"
    printf ' - %s\n' "${invalid_entries[@]}"
    exit 1
  fi

  declare -A manifest_lookup=()
  for manifest_path in "${manifest_paths[@]}"; do
    manifest_lookup["$manifest_path"]=1
  done

  missing_entries=()
  for archive in "${archive_candidates[@]}"; do
    if [[ -z "${manifest_lookup[$archive]+x}" ]]; then
      missing_entries+=("$archive")
    fi
  done

  if (( ${#missing_entries[@]} > 0 )); then
    echo "ERROR: checksum manifest is missing entries for vendored archives:"
    printf ' - %s\n' "${missing_entries[@]}"
    exit 1
  fi

  if ! (cd "$ROOT_DIR" && sha256sum -c <(grep -v -E '^[[:space:]]*(#|$)' "$archive_manifest")); then
    echo "ERROR: vendored archive checksum verification failed."
    exit 1
  fi
fi

echo "Dependency pinning check passed: required real dependencies are pinned via git submodules."
