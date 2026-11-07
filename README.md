# GF Amiga Code::Blocks

Code::Blocks 25.03, the C and C++ IDE, for AmigaOS 3 (68040), on the GFOpenWX Toolkit (wxWidgets on GFOpenLayout).

Code::Blocks is fetched at build time at the version `codeblocks.lock` pins, and `patches/` is applied to it; nothing third-party is kept in this repository.

## What is built

- The IDE with Squirrel scripting and the start page, on our own libraries: the editor is the toolkit's wxStyledTextCtrl (the one Scintilla, in GFOpenWX), and XML goes through OXP, our parser (gfopenxml), under our own TinyXML interface. Code::Blocks' wxScintilla and TinyXML copies are not built.
- The core plugins, linked into the one program (AmigaOS 3 has no shared C++ libraries): compiler, code completion, wizards, abbreviations, AStyle, autosave, class wizard, files extension handler, open files list, occurrences highlighting, projects importer, to-do list.
- `share/codeblocks` as upstream's `make install` lays it out: lexers, compiler definitions, scripts, templates and each plugin's resources.

Left out for now: the debugger plugin (no gdb for the Amiga yet), the contrib plugins, and the separate tools (console runner, share config).

## Building

```
STOVE=~/AmiCradle/stoves/os32-gcc16-open6 OUT=out ./build-amiga.sh
```

The first run fetches the GFOpenWX Toolkit and OXP at the commits `codeblocks.lock` pins and builds its SDK, then fetches Code::Blocks and builds it for what is left of 840 s; run it again and it goes on from where it stopped (the build tree is in `~/.cache/gfamigacodeblocks/build`, outside the checkout). `out/CodeBlocks/` appears when it links: copy the drawer anywhere and run `CodeBlocks` from it. Settings go in its `Config` drawer.

68040, soft float. With `FPUAUTO=` set to a directory holding `fpuauto.specs` and `libfpuauto.a`, the same program uses the FPU when there is one.

| Where | What |
|---|---|
| `patches/` | Code::Blocks' changes for the Amiga: plugins linked in, paths, no wxURL, the app without IPC, tray icon or tips where wx has none |
| `scripts/amake.py` | reads Code::Blocks' Makefile.am files: the sources of each library and plugin, and the data `make install` puts in share/codeblocks (its zips made from their rules) |
| `scripts/build-codeblocks.sh` | fetches, patches, compiles, links and installs |
| `config/build.conf` | the automake conditions that hold and the directories left out |
| `scripts/scicompat.py` | writes the `wx/wxscintilla.h` that puts the editor on the toolkit's wxStyledTextCtrl |
| `deps/tinyxml-oxp/` | our TinyXML interface on OXP: the tree, the printer and the reader |
| `deps/amiga/` | the program's drawer as a full path; wxScintilla's class info |

## ARexx

Code::Blocks has an ARexx port, `CODEBLOCKS.1` (`CODEBLOCKS.2` for a second copy), from the GFOpenWX Toolkit's `wxARexxPort` on arexx.class (patch 0006).

| Command | Does |
| --- | --- |
| `OPEN FILE/A` | open a file, project or workspace, as File > Open |
| `SAVEALL` | save every file and project (RC 5 when one was not saved) |
| `BUILD`, `REBUILD`, `CLEAN`, `RUN` `TARGET,WAIT/S` | the active project, its active target or TARGET; WAIT answers once it has finished (RC 10, RC2 the exit code, when that is not 0) |
| `STOP` | stop the build or the program |
| `BUSY` | RESULT: 1 while building or running, else 0 |
| `QUIT`, `ACTIVATE`, `DEACTIVATE`, `VERSION`, `HELP`, `FAULT` | the toolkit's standard commands |

```
/* Code::Blocks: build hello, then run it */
ADDRESS 'CODEBLOCKS.1'
'OPEN Work:Projects/hello/hello.cbp'
'BUILD WAIT'
IF rc = 0 THEN 'RUN'
ELSE SAY 'build failed, exit code' rc2
```

## Licence

Code::Blocks is GPL v3; the patches and build files here are under the same licence, as is `deps/tinyxml-oxp/`. OXP is MIT.

Designed, tested and delivered by The AmiCradle Team.
Copyright (c) 2026 AmiCradle Team
