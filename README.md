# GF Amiga Code::Blocks

Code::Blocks 25.03, the C and C++ IDE, for AmigaOS 3 (68040), on the GFOpenWX Toolkit (wxWidgets on GFOpenLayout).

Code::Blocks is fetched at build time at the version `codeblocks.lock` pins, and `patches/` is applied to it; nothing third-party is kept in this repository.

## What is built

- The IDE with its own wxScintilla editor, Squirrel scripting, TinyXML and the start page.
- The core plugins, linked into the one program (AmigaOS 3 has no shared C++ libraries): compiler, code completion, wizards, abbreviations, AStyle, autosave, class wizard, files extension handler, open files list, occurrences highlighting, projects importer, to-do list.
- `share/codeblocks` as upstream's `make install` lays it out: lexers, compiler definitions, scripts, templates and each plugin's resources.

Left out for now: the debugger plugin (no gdb for the Amiga yet), the contrib plugins, and the separate tools (console runner, share config).

## Building

```
STOVE=~/AmiCradle/stoves/os32-gcc16-open6 OUT=out ./build-amiga.sh
```

The first run fetches the GFOpenWX Toolkit at the commit `codeblocks.lock` pins and builds its SDK, then fetches Code::Blocks and builds it for what is left of 840 s; run it again and it goes on from where it stopped (the build tree is in `~/.cache/gfamigacodeblocks/build`, outside the checkout). `out/CodeBlocks/` appears when it links: copy the drawer anywhere and run `CodeBlocks` from it. Settings go in its `Config` drawer.

68040, soft float. With `FPUAUTO=` set to a directory holding `fpuauto.specs` and `libfpuauto.a`, the same program uses the FPU when there is one.

| Where | What |
|---|---|
| `patches/` | Code::Blocks' changes for the Amiga: plugins linked in, paths |
| `scripts/amake.py` | reads Code::Blocks' Makefile.am files: the sources of each library and plugin, and the data `make install` puts in share/codeblocks (its zips made from their rules) |
| `scripts/build-codeblocks.sh` | fetches, patches, compiles, links and installs |
| `config/build.conf` | the automake conditions that hold and the directories left out |
| `deps/amiga/` | the program's drawer as a full path |

## Licence

Code::Blocks is GPL v3; the patches and build files here are under the same licence.

Designed, tested and delivered by The AmiCradle Team.
Copyright (c) 2026 AmiCradle Team
