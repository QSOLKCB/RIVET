/* SPDX-License-Identifier: MPL-2.0 */

#include <exec/execbase.h>
#include <proto/exec.h>
#include <stdio.h>

#ifndef RIVET_R9_TARGET_PROFILE
#define RIVET_R9_TARGET_PROFILE "amiga-m68k"
#endif

extern struct ExecBase *SysBase;

int rivet_r9_write_os_identity(FILE *file)
{
    struct Library *dos_base;
    int written;

    if (file == NULL || SysBase == NULL) {
        return 0;
    }

    dos_base = OpenLibrary("dos.library", 0);
    if (dos_base == NULL) {
        return 0;
    }

    written = fprintf(
        file,
        "rivet-r9-os: target=%s os=amigaos "
        "exec_version=%u exec_revision=%u "
        "dos_version=%u dos_revision=%u api=exec-dos\n",
        RIVET_R9_TARGET_PROFILE,
        (unsigned int)SysBase->LibNode.lib_Version,
        (unsigned int)SysBase->LibNode.lib_Revision,
        (unsigned int)dos_base->lib_Version,
        (unsigned int)dos_base->lib_Revision
    );

    CloseLibrary(dos_base);
    return written > 0;
}
