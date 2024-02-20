#!/bin/bash

# extracts pragmas in the form of:
#   #pragma comment(lib, "Shlwapi")
#
# and outputs them as:
#   -lShlwapi

#grep -E "^#pragma comment\( *lib *, \".*\"\)"
#exit

set -e -o pipefail

[[ $1 ]] || {
    echo "$(basename $0): missing first argument. Expected output file."
    exit 1
} >&2

rm -f $1
readarray -t items < <(grep -E "^#pragma comment *\( *lib *, \".*\"\)" | grep -Eo '".*"' | tr -d \")

(( ${#items[@]} <= 0 )) && {
    touch $1
    exit 0
}

# use -d prior to mkdir because it's faster than calling mkdir straight up.
[[ -d "$1" ]] || mkdir -p "$(dirname $1)"

printf -- '-l%s ' "${items[@]}" > $1 || {
    rm -f $1
    exit 1
}

exit 0
