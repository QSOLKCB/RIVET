/* SPDX-License-Identifier: MPL-2.0 */

#include "text_file.h"

#include <stdio.h>

int rivet_headless_read_text_file(
    const char *path,
    unsigned char *buffer,
    size_t capacity,
    size_t *byte_count
)
{
    FILE *file;
    size_t used = 0u;
    int ch;

    if (path == NULL || path[0] == '\0' ||
        buffer == NULL || capacity == 0u ||
        byte_count == NULL) {
        return 0;
    }

    file = fopen(path, "rb");
    if (file == NULL) {
        return 0;
    }

    while ((ch = fgetc(file)) != EOF) {
        if (used == capacity) {
            fclose(file);
            return 0;
        }
        buffer[used++] = (unsigned char)ch;
    }

    if (ferror(file)) {
        fclose(file);
        return 0;
    }
    if (fclose(file) != 0) {
        return 0;
    }

    *byte_count = used;
    return 1;
}
