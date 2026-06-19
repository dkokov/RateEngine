#!/bin/sh
#
# Fetch the DuckDB C/C++ library bundle (duckdb.h + libduckdb.so +
# libduckdb_static.a) into a destination dir. Idempotent: if the files are
# already there it does nothing, so it is safe to call from the build.
#
# The binaries are NOT kept in git (see .gitignore); a fresh clone downloads
# them on first build. The version is pinned in <dest>/../DUCKDB_VERSION.
#
# Usage: fetch_duckdb.sh <dest_dir> [version] [arch]
#
set -eu

DEST="${1:?usage: fetch_duckdb.sh <dest_dir> [version] [arch]}"

# version: arg > <dest>/../DUCKDB_VERSION > fallback
VER="${2:-}"
if [ -z "$VER" ]; then
	if [ -f "$DEST/../DUCKDB_VERSION" ]; then
		VER="$(tr -d ' \t\r\n' < "$DEST/../DUCKDB_VERSION")"
	else
		VER="1.5.3"
	fi
fi

ARCH="${3:-$(uname -m)}"
case "$ARCH" in
	x86_64|amd64)  PKG="amd64" ;;
	aarch64|arm64) PKG="arm64" ;;
	*) echo "fetch_duckdb: unsupported arch '$ARCH'" >&2; exit 1 ;;
esac

# already present? (need the header and at least one of the libs)
if [ -f "$DEST/duckdb.h" ] && { [ -f "$DEST/libduckdb.so" ] || [ -f "$DEST/libduckdb_static.a" ]; }; then
	echo "fetch_duckdb: already present in $DEST (pinned v$VER)"
	exit 0
fi

URL="https://github.com/duckdb/duckdb/releases/download/v${VER}/libduckdb-linux-${PKG}.zip"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

echo "fetch_duckdb: downloading DuckDB v$VER ($PKG)"
echo "  $URL"
if command -v curl >/dev/null 2>&1; then
	curl -fSL "$URL" -o "$TMP/libduckdb.zip"
elif command -v wget >/dev/null 2>&1; then
	wget -q "$URL" -O "$TMP/libduckdb.zip"
else
	echo "fetch_duckdb: need curl or wget" >&2; exit 1
fi

mkdir -p "$DEST"
unzip -o "$TMP/libduckdb.zip" -d "$DEST" >/dev/null

echo "fetch_duckdb: installed into $DEST:"
ls -1 "$DEST"
