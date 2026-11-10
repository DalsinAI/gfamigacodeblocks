#!/bin/sh
# Fetches Code::Blocks at its pinned release, applies patches/, and builds
# it against gfopenwx's SDK: the SDK, wxScintilla, Squirrel, TinyXML and the
# core plugins as libraries (the sources each Makefile.am lists, read by
# amake.py), the plugins linked into the one program, and share/codeblocks
# as make install lays it out. The build tree is kept, so a build that runs
# out of time goes on from where it stopped (BUDGET seconds of make a run).
#   TOOLCHAIN=.../prefix WX_SDK=... WORK=build OUT=out scripts/build-codeblocks.sh
#   OXP_SRC=gfopenxml's oxp/: our XML parser, under our TinyXML interface
#   FPUAUTO=dir with fpuauto.specs and libfpuauto.a: the FPU when there is one
# Copyright (c) 2026 AmiCradle Team
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
. "$HERE/codeblocks.lock"
WORK=${WORK:-$HERE/build}; OUT=${OUT:-$WORK/out}; JOBS=${JOBS:-12}; BUDGET=${BUDGET:-700}
PATH="$TOOLCHAIN/bin:$PATH"
SRC="$WORK/codeblocks"; B="$WORK/cbbuild"; D="$WORK/dl"
mkdir -p "$OUT" "$D"

# the release tarball, checked by sha256, then patches/ (and EXTRA_PATCHES,
# a directory of lab-only ones, last)
if [ ! -f "$D/codeblocks.tar.xz" ] || ! echo "$CB_SHA256  $D/codeblocks.tar.xz" | sha256sum -c - >/dev/null 2>&1; then
    curl -fsSL -o "$D/codeblocks.tar.xz.part" "$CB_URL"
    echo "$CB_SHA256  $D/codeblocks.tar.xz.part" | sha256sum -c - >/dev/null || { echo "sha256 mismatch: $CB_URL"; exit 1; }
    mv "$D/codeblocks.tar.xz.part" "$D/codeblocks.tar.xz"
fi
EXTRA=""; [ -n "${EXTRA_PATCHES:-}" ] && EXTRA=$(ls "$EXTRA_PATCHES"/*.patch 2>/dev/null || true)
PSUM=$(cat "$HERE"/patches/*.patch $EXTRA "$HERE/scripts/scicompat.py" "$HERE"/deps/tinyxml-oxp/* | sha256sum | cut -c1-16)
if [ "$(cat "$SRC/.patched" 2>/dev/null)" != "$CB_SHA256-$PSUM" ]; then
    # a fresh tree, patched, then copied over the old one by content, so
    # only the files a patch change touches are compiled again
    N="$SRC.new"; rm -rf "$N"; mkdir -p "$N"
    tar xJf "$D/codeblocks.tar.xz" -C "$N" --strip-components=1
    for p in "$HERE"/patches/*.patch $EXTRA; do
        patch -s -d "$N" -p1 < "$p" || { echo "patch failed: $p"; exit 1; }
    done
    # what configure and build_tools/autorevision would write
    sed -e "s|@REVISION@|$CB_REVISION|g" -e "s|@DATE@|$CB_DATE|g" \
        "$N/src/include/autorevision.h.in" > "$N/src/include/autorevision.h"
    # the editor on the toolkit's wxStyledTextCtrl, not the wxScintilla copy
    # (config/build.conf skips its directory)
    SCI="$N/src/sdk/wxscintilla/include/wx/wxscintilla.h"
    python3 -I "$HERE/scripts/scicompat.py" "$SCI" "$SCI.new" && mv "$SCI.new" "$SCI"
    # its event is wxStyledTextEvent by another name, so the headers that
    # only declare the class name it that way
    grep -rlE '^class wxScintillaEvent;' "$N/src" | while read -r f; do
        sed -i 's/^class wxScintillaEvent;/class wxStyledTextEvent; typedef wxStyledTextEvent wxScintillaEvent;/' "$f"
    done
    # XML through OXP: our TinyXML interface in place of the TinyXML copy
    cp "$HERE/deps/tinyxml-oxp/tinyxml.h" "$N/src/include/tinyxml/tinyxml.h"
    echo '#include "tinyxml.h"' > "$N/src/include/tinyxml/tinystr.h"
    cp "$HERE/deps/tinyxml-oxp/tinyxml.cpp" "$N/src/base/tinyxml/tinyxml.cpp"
    for f in tinystr tinyxmlerror tinyxmlparser; do
        echo '// in tinyxml.cpp (deps/tinyxml-oxp)' > "$N/src/base/tinyxml/$f.cpp"
    done
    if [ -d "$SRC" ] && command -v rsync >/dev/null; then
        rsync -a --checksum --delete --exclude=.patched "$N/" "$SRC/"; rm -rf "$N"
    else
        rm -rf "$SRC"; mv "$N" "$SRC"
    fi
    echo "$CB_SHA256-$PSUM" > "$SRC/.patched"
fi

# the rules, made again when the sources, the patches or our reader change
W="$WX_SDK/bin/wx-config"
LATE=""
if m68k-amigaos-gcc -fno-late-combine-instructions -S -o /dev/null -x c /dev/null 2>/dev/null; then LATE="-fno-late-combine-instructions"; fi
CF="-noixemul -m68040 -O2 $LATE"
DEFS="-DTIXML_USE_STL=YES -I$SRC/src/include/tinyxml -I$OXP_SRC/include"
MSUM=$( (cat "$SRC/.patched" "$HERE/scripts/amake.py" "$HERE/config/build.conf" "$HERE/scripts/scicompat.py" "$0"; "$W" --cxxflags) | sha256sum | cut -c1-16)
mkdir -p "$B"
if [ "$(cat "$B/.rules" 2>/dev/null)" != "$MSUM" ]; then
    python3 -I "$HERE/scripts/amake.py" "$SRC" "$HERE/config/build.conf" "$B" "$B/cb.mk"
    {
        echo "CC = m68k-amigaos-gcc"
        echo "CXX = m68k-amigaos-g++"
        echo "AR = m68k-amigaos-ar"
        echo "WX_CXXFLAGS = $("$W" --cxxflags)"
        echo "WX_CFLAGS = \$(WX_CXXFLAGS)"
        echo "CBCFLAGS = $CF $DEFS"
        echo "CBCXXFLAGS = $CF -std=gnu++17 -fexceptions $DEFS -Wno-deprecated-declarations"
        echo "include cb.mk"
        echo "obj/amiga/progdir.o: $HERE/deps/amiga/progdir.c"
        printf '\t@mkdir -p $(@D)\n\t$(CC) $(CBCFLAGS) -c $< -o $@\n'
        echo "obj/oxp/oxp.o: $OXP_SRC/src/oxp.c"
        printf '\t@mkdir -p $(@D)\n\t$(CC) $(CBCFLAGS) -std=gnu99 -c $< -o $@\n'
        echo "obj/amiga/wxscintilla.o: $HERE/deps/amiga/wxscintilla.cpp"
        printf '\t@mkdir -p $(@D)\n\t$(CXX) $(CBCXXFLAGS) $(WX_CXXFLAGS) -I%s/src/sdk/wxscintilla/include -c $< -o $@\n' "$SRC"
        echo "all: \$(CB_LIBS) \$(CB_PLUGINS) \$(codeblocks_OBJS) obj/amiga/progdir.o obj/amiga/wxscintilla.o obj/oxp/oxp.o link.args"
        echo "link.args: Makefile cb.mk"
        printf '\t@echo $(codeblocks_OBJS) obj/amiga/progdir.o obj/amiga/wxscintilla.o obj/oxp/oxp.o -Wl,--whole-archive $(CB_PLUGINS) -Wl,--no-whole-archive -Wl,--start-group $(CB_LIBS) -Wl,--end-group > $@\n'
    } > "$B/Makefile"
    echo "$MSUM" > "$B/.rules"
fi

# make for at most BUDGET seconds; -k so one bad file shows the rest
set +e
(cd "$B" && timeout "$BUDGET" nice -n 19 make -k -j"$JOBS" all > make.log 2>&1)
rc=$?
set -e
echo "make rc=$rc"
grep -E 'error:|Error [0-9]' "$B/make.log" | sort -u | head -60
tail -5 "$B/make.log"
[ "$rc" = 0 ] || exit 0

# the one program: the plugins whole (their registrants are all that
# reaches them), the SDK and the rest in one group with wx
SPECS="-specs=$HERE/scripts/m68k-soft-m68040.specs"
FPU=""
if [ -n "${FPUAUTO:-}" ]; then SPECS="$SPECS -specs=$FPUAUTO/fpuauto.specs"; FPU="-L$FPUAUTO"; fi
cd "$B"
# shellcheck disable=SC2046
m68k-amigaos-g++ -noixemul -m68040 $SPECS $FPU -o codeblocks $(cat link.args) $("$W" --libs std,aui,xrc,html,propgrid,richtext) > link.log 2>&1 || { grep -E 'undefined reference|multiple definition|error' link.log | sort -u | head -60; exit 1; }
cd - > /dev/null

# the install: CodeBlocks/ with the program and share/codeblocks
I="$OUT/CodeBlocks"
rm -rf "$I"; mkdir -p "$I/Config"
m68k-amigaos-strip -o "$I/CodeBlocks" "$B/codeblocks" || cp "$B/codeblocks" "$I/CodeBlocks"
python3 -I "$HERE/scripts/amake.py" --data "$SRC" "$HERE/config/build.conf" "$I/share"
# the ARexx example (README: ARexx)
mkdir -p "$I/Rexx"; cp "$HERE"/Rexx/*.rexx "$I/Rexx/"
mkdir -p "$I/tests"; cp "$HERE/tests/arexx.rexx" "$I/tests/"   # the ARexx port test
ls -l "$B/codeblocks" "$I/CodeBlocks"
du -sh "$I"

# the release: the drawer's icon, ReadMe and licence, then the LhA archive
python3 -I "$HERE/scripts/make-icon.py" "$I/CodeBlocks.info"
cp "$HERE/README.md" "$I/ReadMe"; cp "$HERE/LICENSE" "$I/LICENSE"
. "$HERE/release.lock"
python3 -I "$HERE/tools/mklha.py" "$I" "$OUT/GFAmigaCodeBlocks-$RELEASE.lha"
( cd "$OUT" && sha256sum "GFAmigaCodeBlocks-$RELEASE.lha" > SHA256SUMS && cat SHA256SUMS )
