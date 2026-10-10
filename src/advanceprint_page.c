#include <string.h>
#include "advanceprint/advanceprint_page.h"
#include "advanceprint/advanceprint_font.h"

static const uint8_t *s_font = ap_default_font;
static int s_glyphs = AP_FONT_GLYPHS;

void ap_page_set_font(const uint8_t *font_2bpp, int glyph_count)
{
    if (font_2bpp && glyph_count > 0) {
        s_font = font_2bpp;
        s_glyphs = glyph_count;
    } else {
        s_font = ap_default_font;
        s_glyphs = AP_FONT_GLYPHS;
    }
}

void ap_page_clear(uint8_t *page)
{
    memset(page, 0, AP_PAGE_BYTES);
}

void ap_page_render_row(const char *chars, int n, uint8_t *out)
{
    for (int t = 0; t < AP_TILES_PER_ROW; t++) {
        if (t < n) {
            unsigned idx = (uint8_t)chars[t] & 0x7F;
            if ((int)idx >= s_glyphs)
                idx = 0;
            memcpy(out + t * 16, s_font + idx * AP_FONT_GLYPH_BYTES,
                   AP_FONT_GLYPH_BYTES);
        } else {
            memset(out + t * 16, 0, AP_FONT_GLYPH_BYTES); // shade 0 = paper
        }
    }
}

void ap_page_compose_text(uint8_t *page, const char *text, const ApText *t,
                          int first_line)
{
    for (int row = 0; row < AP_ROWS_PER_PAGE; row++) {
        int ln = first_line + row;
        uint8_t *out = page + (size_t)row * AP_ROW_BYTES;
        if (ln < t->count) {
            int llen = ap_text_line_len(t, text, ln);
            if (llen > AP_TILES_PER_ROW)
                llen = AP_TILES_PER_ROW;  // wrap-overflow spaces are clipped
            ap_page_render_row(text + t->start[ln], llen, out);
        } else {
            memset(out, 0, AP_ROW_BYTES);
        }
    }
}
