// wxLaunchDefaultApplication() for the openlayout toolkit until the toolkit
// has its own: Workbench opens the file as a double-click would, with its
// icon's default tool (workbench.library 44, OpenWorkbenchObjectA()).
// Drop this file when gfopenwxtoolkit provides the function.
// Copyright (c) 2026 AmiCradle Team
#include <wx/string.h>
#include <proto/exec.h>
#include <proto/workbench.h>

struct Library *WorkbenchBase;

bool wxLaunchDefaultApplication(const wxString& document, int flags)
{
    (void)flags;
    bool ok = false;
    struct Library *wb = OpenLibrary((CONST_STRPTR)"workbench.library", 44);
    if (wb) {
        struct Library *old = WorkbenchBase;
        WorkbenchBase = wb;
        const wxScopedCharBuffer name = document.mb_str();
        ok = OpenWorkbenchObjectA((STRPTR)name.data(), NULL) != FALSE;
        WorkbenchBase = old;
        CloseLibrary(wb);
    }
    return ok;
}
