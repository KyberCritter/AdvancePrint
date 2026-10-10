// Bundled default GB 2bpp print font (128 glyphs, 16 bytes each).
// Generated for GBScribe by tools/mkfont.py; glyph codes 0x20..0x7E are ASCII,
// specials live in the C0 range.
#ifndef AP_FONT_H
#define AP_FONT_H

#include <stdint.h>

#define AP_FONT_GLYPHS      128
#define AP_FONT_GLYPH_BYTES 16

extern const uint8_t ap_default_font[AP_FONT_GLYPHS * AP_FONT_GLYPH_BYTES];

#endif
