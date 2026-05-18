#!/usr/bin/env bash
# Compile-all validation for NimBLE examples.
# Exits non-zero if any sketch fails. Prints a summary at the end.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FQBN="esp32:esp32:esp32"
FAIL=()
OK=0
TOTAL=0
while IFS= read -r -d '' sketch_dir; do
  TOTAL=$((TOTAL + 1))
  name="${sketch_dir#$ROOT/}"
  printf "[%2d] %-70s " "$TOTAL" "$name"
  if arduino-cli compile --fqbn "$FQBN" --warnings all "$sketch_dir" \
       >/tmp/nimble-compile.log 2>&1; then
    echo "OK"
    OK=$((OK + 1))
  else
    echo "FAIL"
    FAIL+=("$name")
    tail -20 /tmp/nimble-compile.log | sed 's/^/    | /'
  fi
done < <(find "$ROOT" -mindepth 2 -name '*.ino' -printf '%h\0' | sort -z)
echo
echo "Result: $OK/$TOTAL OK"
if (( ${#FAIL[@]} > 0 )); then
  echo "Failures:"
  printf '  - %s\n' "${FAIL[@]}"
  exit 1
fi
