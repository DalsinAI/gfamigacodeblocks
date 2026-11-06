/* The program's drawer as a full path ("Work:CodeBlocks"), for Code::Blocks'
 * paths: they are made as dir + "/" + name, which on AmigaOS would climb
 * to the parent if dir were PROGDIR:. Patch 0002 calls it.
 * Copyright (c) 2026 AmiCradle Team */
#include <string.h>
#include <proto/dos.h>

const char *amiga_program_dir(void)
{
    static char path[512];
    if (!path[0]) {
        BPTR lock = GetProgramDir();
        if (!lock || !NameFromLock(lock, (STRPTR)path, sizeof(path)))
            strcpy(path, "PROGDIR:");
    }
    return path;
}
