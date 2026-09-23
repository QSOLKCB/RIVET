/* SPDX-License-Identifier: MPL-2.0 */
#ifndef RIVET_HEADLESS_TEXT_FILE_H
#define RIVET_HEADLESS_TEXT_FILE_H

#include <stddef.h>

int rivet_headless_read_text_file(
    const char *path,
    unsigned char *buffer,
    size_t capacity,
    size_t *byte_count
);

#endif
