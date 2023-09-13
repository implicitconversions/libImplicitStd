#!/bin/bash
# Helper script for direct use from incremental_build_support.mk


m_compile_file=$1

hash=$(sha256sum <<< "$tmp_makeincr_COMPILE" | cut -c-64)
basedir=$(dirname $m_compile_file)
cmp -s $m_compile_file <(echo "$hash") || {
    [[ -f "$m_compile_file" ]] && {
        echo >&2 "compile flags changed: $m_compile_file"
    }
    [[ -d "$basedir" ]] || mkdir -p "$basedir"
    echo "$hash" > $m_compile_file
}
