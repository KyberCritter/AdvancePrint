// Page composition: text or raw pixels -> 160px-wide GB 2bpp tile page.
// PORTABLE C — no GBA headers.
#ifndef AP_PAGE_H
#define AP_PAGE_H

#include <stdint.h>
#include "advanceprint/advanceprint_text.h"

#define AP_TILES_PER_ROW 20                                   // 160 px wide
#define AP_ROWS_PER_PAGE 18                                   // 144 px tall
#define AP_ROW_BYTES     (AP_TILES_PER_ROW * 16)              // 320
#define AP_PAGE_BYTES    (AP_ROW_BYTES * AP_ROWS_PER_PAGE)    // 5760
#define AP_DATA_PER_PKT  0x280                                // 2 tile rows
#define AP_PKTS_PER_PAGE (AP_PAGE_BYTES / AP_DATA_PER_PKT)    // 9

// Install a custom GB 2bpp font (glyph_count glyphs, 16 bytes each, indexed by
// char & 0x7F). Pass NULL to restore the bundled default font.
void ap_page_set_font(const uint8_t *font_2bpp, int glyph_count);

// Zero a whole page (all pixels = shade 0 = white paper).
void ap_page_clear(uint8_t *page);

// Render up to `n` glyphs into one tile row (AP_ROW_BYTES), blank-padded.
void ap_page_render_row(const char *chars, int n, uint8_t *out);

// Compose one printed page (AP_PAGE_BYTES) from a wrapped text table starting
// at layout line `first_line`. Lines past the end of the text become blank.
void ap_page_compose_text(uint8_t *page, const char *text, const ApText *t,
                          int first_line);

#endif
