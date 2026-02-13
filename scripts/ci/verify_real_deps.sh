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

echo "Dependency pinning check passed: required real dependencies are pinned via git submodules."
