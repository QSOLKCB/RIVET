/* SPDX-License-Identifier: MPL-2.0 */
#ifndef RIVET_HEADLESS_PPM_H
#define RIVET_HEADLESS_PPM_H

#include "rivet/gfx.h"

int rivet_headless_write_ppm(
    const char *path,
    const rivet_surface *surface
);

#endif
