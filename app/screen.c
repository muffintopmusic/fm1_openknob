/* SPDX-License-Identifier: GPL-3.0-only WITH GPL-3.0-linking-exception */
#include "instrument.h"
#include "fm_patch.h"
#include "version.h"
#include "fm1_engine_control.h"
#include <string.h>

enum {
    BACK = 0x0000,
    PANEL = 0x18e3,
    TEXT = 0xefe5d,
    ACCENT = 0x07ff,
    DIM = 0x738e,
    GREEN = 0x07e0,
    AMBER = 0xfd20
};
/* Five columns per glyph, bit 0 at the top. Digits followed by uppercase. */
static const uint8_t font3x6[36][5] = {
    ...
};

static void read_encoders(const struct fm1_controls *controls, uint32_t now, unsigned boundary)
{
    /* If HOME is not held, let the select encoder behave like knob 1.
     * While HOME is held, keep the original select knob behavior intact. */
    struct fm1_controls mapped = *controls;
    if (!(mapped.buttons & BUTTON(HOME))) {
        int16_t tmp = mapped.encoders[0];
        mapped.encoders[0] = mapped.encoders[6];
        mapped.encoders[6] = tmp;
    }

    /* Apply the first detent immediately. A continuing fast turn accelerates
     * only continuous parameters; page/option selectors retain raw detents.
     * Reversal, a pause or a new control context returns to fine adjustment. */
    if (boundary) memset(encoder_direction, 0, sizeof encoder_direction);
    for (unsigned i = 0; i < 7; ++i) {
        int direction = (mapped.encoders[i] > 0) - (mapped.encoders[i] < 0);
        if (!direction) continue;
        encoder_speed[i] = direction == encoder_direction[i] && now - encoder_time[i] <= 40U
            ? (uint8_t)clamp(encoder_speed[i] + 1, 1, 4) : 1;
        encoder_direction[i] = (int8_t)direction;
        encoder_time[i] = now;
    }
}

