#!/bin/sh
#
# gen_prompts.sh — generate core/pd/<name>.c from prompts/<name>.md.
#
# Each generated file defines:
#   const char P_<UPPER>[] = "line1\n" "line2\n" ...;
#
# The generator is idempotent; running it twice produces the same
# bytes. Generated files are committed so the repo builds without
# bash. Re-run via `make prompts` after editing any prompts/*.md.
#
set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
SRC="$ROOT/prompts"
DST="$ROOT/core/pd"
mkdir -p "$DST"

# Map a relative path ("system/persona.md") to a C identifier
# ("SYSTEM_PERSONA") and a filename ("system_persona").
emit_one() {
    rel=$1
    base=${rel%.md}
    id=$(printf '%s' "$base" | tr '[:lower:]/.' '[:upper:]__')
    fn=$(printf '%s' "$base" | tr '/.' '__')
    out="$DST/$fn.c"
    {
        printf '/* auto-generated from prompts/%s — do not edit by hand */\n' "$rel"
        printf '#include "core/pd.h"\n\n'
        printf 'const char P_%s[] =\n' "$id"
        # Escape each markdown line as a C string literal.
        # Backslash, doublequote, then append \n per line.
        awk 'BEGIN{OFS=""} {
            gsub(/\\/, "\\\\")
            gsub(/"/, "\\\"")
            print "    \"" $0 "\\n\""
        }' "$SRC/$rel"
        printf ';\n'
    } > "$out.tmp"
    mv "$out.tmp" "$out"
}

# Collect relative paths under prompts/ and emit one C file each.
cd "$SRC"
find . -type f -name '*.md' | sed 's|^\./||' | sort | while read -r rel; do
    emit_one "$rel"
done

# Emit the header listing every symbol so the registry can extern them.
hdr="$ROOT/core/pd.h"
{
    printf '/* auto-generated — do not edit by hand */\n'
    printf '#ifndef CORE_PD_H\n#define CORE_PD_H\n\n'
    cd "$SRC"
    find . -type f -name '*.md' | sed 's|^\./||' | sort | while read -r rel; do
        base=${rel%.md}
        id=$(printf '%s' "$base" | tr '[:lower:]/.' '[:upper:]__')
        printf 'extern const char P_%s[];\n' "$id"
    done
    printf '\n#endif\n'
} > "$hdr.tmp"
mv "$hdr.tmp" "$hdr"

echo "gen_prompts.sh: wrote $(ls "$DST"/*.c | wc -l) prompts"
