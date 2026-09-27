/* SPDX-License-Identifier: MPL-2.0 */

#include <Gestalt.h>
#include <stdio.h>

#ifndef RIVET_R9_TARGET_PROFILE
#define RIVET_R9_TARGET_PROFILE "classic-mac-unknown"
#endif

int rivet_r9_write_os_identity(FILE *file)
{
    long system_version = 0;
    unsigned int major;
    unsigned int minor;
    unsigned int patch;

    if (file == NULL) {
        return 0;
    }
    if (Gestalt(
            gestaltSystemVersion,
            &system_version) != 0) {
        return 0;
    }

    major = (unsigned int)(
        ((unsigned long)system_version >> 8) & 0xfful
    );
    minor = (unsigned int)(
        ((unsigned long)system_version >> 4) & 0x0ful
    );
    patch = (unsigned int)(
        (unsigned long)system_version & 0x0ful
    );

    return fprintf(
        file,
        "rivet-r9-os: target=%s os=classic-mac-os "
        "version=%u.%u.%u api=toolbox\n",
        RIVET_R9_TARGET_PROFILE,
        major,
        minor,
        patch
    ) > 0;
}
