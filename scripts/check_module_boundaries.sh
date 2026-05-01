#!/bin/bash
set -euo pipefail

ERRORS=0

check_no_include() {
    local dir="$1"
    shift
    local forbidden_patterns=("$@")

    if [[ ! -d "$dir" ]]; then
        return
    fi

    for pattern in "${forbidden_patterns[@]}"; do
        matches=$(grep -rn "#include.*\"${pattern}" "$dir" --include="*.cpp" --include="*.h" \
            | grep -v "// KNOWN_VIOLATION" || true)
        if [[ -n "$matches" ]]; then
            echo "VIOLATION: Files in $dir include '$pattern':"
            echo "$matches"
            ERRORS=$((ERRORS + 1))
        fi
    done
}

cd "$(dirname "$0")/.."

check_no_include "source/game" "editor/" "ui/" "rendering/"
check_no_include "source/map" "editor/" "ui/" "rendering/"
check_no_include "source/util" "editor/" "ui/" "rendering/" "game/" "map/" "brushes/" "io/" "live/" "net/" "app/"
check_no_include "source/rendering" "ui/" "brushes/"
check_no_include "source/io" "rendering/"

IO_UI_VIOLATIONS=$(grep -rn '#include.*"ui/' source/io --include="*.cpp" --include="*.h" \
    | grep -v "// KNOWN_VIOLATION" \
    | grep -v "iomap_otbm.cpp" || true)
if [[ -n "$IO_UI_VIOLATIONS" ]]; then
    echo "VIOLATION: Files in source/io include ui/ headers (excluding known iomap_otbm.cpp):"
    echo "$IO_UI_VIOLATIONS"
    ERRORS=$((ERRORS + 1))
fi

if [[ "$ERRORS" -gt 0 ]]; then
    echo ""
    echo "Found $ERRORS module boundary violation(s)."
    exit 1
fi

echo "All module boundary checks passed."
exit 0