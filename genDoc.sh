#!/bin/sh
#
# genDoc.sh - build the RateEngine V7 manual (PDF or HTML) from the .md sources.
#
#   ./genDoc.sh          -> rt_v7.pdf
#   ./genDoc.sh html     -> rt_v7.html  (self-contained, images inlined)
#   ./genDoc.sh all      -> both
#
# Requires: pandoc, and for PDF a LaTeX engine (xelatex / lualatex / pdflatex).
#

set -e

DIR=$(cd "$(dirname "$0")" && pwd)
cd "$DIR"

OUT=rt_v7
TITLE="RateEngine V7"
AUTHOR="Dimitar Kokov"

# Explicit reading order - do NOT use doc/*.md, the alphabetical order is wrong.
DOCS="
README.md
doc/install.md
doc/features.md
doc/arch.md
doc/core_architecture.md
doc/core_details.md
doc/config.md
doc/cdrm.md
doc/cdr_profile.md
doc/call_control.md
doc/cc_int_prof.md
doc/ast_cc.md
doc/fs_cc.md
doc/rating.md
LICENSE.md
"

if ! command -v pandoc >/dev/null 2>&1; then
    echo "genDoc.sh: 'pandoc' is not installed." >&2
    echo "  Fedora/RHEL : sudo dnf install pandoc" >&2
    echo "  Debian/Ubu. : sudo apt install pandoc" >&2
    exit 1
fi

# Warn about referenced images that do not exist (pandoc only prints a warning
# and silently drops them, which is easy to miss).
check_images()
{
    for f in $DOCS; do
        base=$(dirname "$f")
        grep -o '](\(png/\|doc/png/\)[^)]*)' "$f" 2>/dev/null |
        sed 's/^](//; s/)$//' |
        while read -r img; do
            [ -f "$img" ] || [ -f "$base/$img" ] || \
                echo "genDoc.sh: WARNING missing image '$img' (referenced in $f)" >&2
        done
    done
}

# Common pandoc options.
#   --resource-path : doc/*.md reference images as png/... relative to doc/
#   --lua-filter    : strip remote/SVG images (badges) that break the PDF build
# NOTE: COMMON is used unquoted (word splitting is intended), so it must not
# contain any value with spaces - $TITLE/$AUTHOR are passed separately.
COMMON="--from=gfm
        --toc --toc-depth=3
        --number-sections
        --top-level-division=chapter
        --resource-path=.:doc
        --lua-filter=doc/filters/no-remote-images.lua
        --metadata=lang:en"

pick_pdf_engine()
{
    for e in xelatex lualatex pdflatex tectonic; do
        if command -v "$e" >/dev/null 2>&1; then
            echo "$e"
            return 0
        fi
    done
    return 1
}

genDoc_pdf()
{
    engine=$(pick_pdf_engine) || {
        echo "genDoc.sh: no LaTeX engine found (xelatex/lualatex/pdflatex)." >&2
        echo "  Fedora/RHEL : sudo dnf install texlive-scheme-medium" >&2
        echo "  Debian/Ubu. : sudo apt install texlive-xetex texlive-fonts-recommended" >&2
        exit 1
    }
    echo "genDoc.sh: building $OUT.pdf with $engine ..."
    # The default LaTeX mono font has no box-drawing glyphs (the ASCII tree
    # diagrams in the docs). xelatex/lualatex can use a system font instead.
    FONTOPT=""
    case "$engine" in
        xelatex|lualatex)
            for mf in "DejaVu Sans Mono" "Liberation Mono" "Noto Sans Mono"; do
                if fc-list : family 2>/dev/null | grep -qF "$mf"; then
                    FONTOPT="-Vmonofont=$mf"
                    break
                fi
            done
            ;;
    esac
    # shellcheck disable=SC2086
    pandoc $COMMON ${FONTOPT:+"$FONTOPT"} \
        --metadata=title:"$TITLE" --metadata=author:"$AUTHOR" \
        --pdf-engine="$engine" \
        -V documentclass=report \
        -V geometry:margin=2.5cm \
        -V colorlinks=true \
        -V linkcolor=blue \
        -V urlcolor=blue \
        -V toccolor=black \
        $DOCS -o "$OUT.pdf"
    echo "genDoc.sh: wrote $DIR/$OUT.pdf"
}

genDoc_html()
{
    echo "genDoc.sh: building $OUT.html ..."
    # shellcheck disable=SC2086
    pandoc $COMMON \
        --metadata=title:"$TITLE" --metadata=author:"$AUTHOR" \
        --standalone --embed-resources \
        --toc --css=/dev/null \
        $DOCS -o "$OUT.html"
    echo "genDoc.sh: wrote $DIR/$OUT.html"
}

check_images

case "${1:-pdf}" in
    pdf)  genDoc_pdf ;;
    html) genDoc_html ;;
    all)  genDoc_pdf; genDoc_html ;;
    *)    echo "usage: $0 [pdf|html|all]" >&2; exit 1 ;;
esac
