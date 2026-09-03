#!/bin/bash
#
# scripts/build_deb.sh
# Build Debian packages using dpkg-buildpackage (standardized deb packaging).
# Builds in a temporary directory to avoid polluting the source tree.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${SCRIPT_DIR%/*}"
DEB_DIR="$SCRIPT_DIR/debian"
DEB_OUTPUT="$PROJECT_ROOT/build/deb"

BUILD_TMP=$(mktemp -d)
trap "rm -rf $BUILD_TMP $PROJECT_ROOT/debian" EXIT

echo ">>> Preparing temporary build directory..."

rsync -a --exclude='.git' --exclude='build' "$PROJECT_ROOT/" "$BUILD_TMP/"

cp -r "$DEB_DIR" "$BUILD_TMP/debian"
chmod -R u+w "$BUILD_TMP/debian"

rm -rf "$DEB_OUTPUT"
mkdir -p "$DEB_OUTPUT"

echo ">>> Building packages with dpkg-buildpackage..."
cd "$BUILD_TMP"
dpkg-buildpackage -b -us -uc

echo ">>> Collecting output packages..."
shopt -s nullglob

BUILD_PARENT="$(dirname "$BUILD_TMP")"
for ext in deb ddeb changes buildinfo; do
    for f in "$BUILD_PARENT"/*."$ext"; do
        cp "$f" "$DEB_OUTPUT/"
    done
done

shopt -u nullglob

echo ">>> All .deb packages built in $DEB_OUTPUT/"
ls -lh "$DEB_OUTPUT/"
