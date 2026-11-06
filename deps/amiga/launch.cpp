// wxLaunchDefaultApplication() for the openlayout toolkit until the toolkit
// has its own: Workbench opens the file as a double-click would, with its
// icon's default tool (workbench.library 44, OpenWorkbenchObjectA()), where
// the NDK in use declares it; otherwise it reports failure, as wx does when
// no application is found. Drop this file when gfopenwxtoolkit provides it.
// Copyright (c) 2026 AmiCradle Team
#include <wx/string.h>

#if defined(__has_include)
#if __has_include(<proto/workbench.h>)
#define CB_HAVE_WORKBENCH 1
#endif
#endif

#ifdef CB_HAVE_WORKBENCH
#include <proto/exec.h>
#include <proto/workbench.h>
struct Library *WorkbenchBase;
#endif

bool wxLaunchDefaultApplication(const wxString& document, int flags)
{
    (void)flags;
#ifdef CB_HAVE_WORKBENCH
    bool ok = false;
    struct Library *wb = OpenLibrary((CONST_STRPTR)"workbench.library", 44);
    if (wb) {
        WorkbenchBase = wb;
        const wxScopedCharBuffer name = document.mb_str();
        ok = OpenWorkbenchObjectA((STRPTR)name.data(), NULL) != FALSE;
        WorkbenchBase = NULL;
        CloseLibrary(wb);
    }
    return ok;
#else
    (void)document;
    return false;
#endif
}
