#!/usr/bin/env bash
set -euo pipefail
p1='TODO\(PROD\)'
p2='PLACE''HOLDER'
p3='DUM''MY'
p4='MO''CK'
p5='FIXME\(PROD\)'
PATTERN="${p1}|${p2}|${p3}|${p4}|${p5}"
PATHS=(layer1-talanton layer2-drachma layer3-obolos common relayers cli scripts configs)
if command -v rg >/dev/null 2>&1; then
  SEARCH_CMD=(rg -n "$PATTERN" "${PATHS[@]}")
else
  SEARCH_CMD=(grep -r -n -E "$PATTERN" "${PATHS[@]}")
fi

if "${SEARCH_CMD[@]}"; then
  echo "Placeholder gate failed: forbidden markers found"
  exit 1
fi
echo "Placeholder gate passed"
