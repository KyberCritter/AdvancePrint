// Minimal word-wrap engine for printed text. PORTABLE C — no GBA headers.
// Keeps the library's text path self-contained; callers that already have their
// own line table can ignore this and feed ap_page_compose_text directly.
#ifndef AP_TEXT_H
#define AP_TEXT_H

#include <stdint.h>

// Maximum number of wrapped lines held by an ApText. Overridable per build
// (define before including this header) to trade RAM for longer documents.
#ifndef AP_TEXT_MAX_LINES
#define AP_TEXT_MAX_LINES 4096
#endif

typedef enum {
    AP_WRAP_MID_WORD,   // hard break exactly at the column limit
    AP_WRAP_POST_WORD,  // break after the last full word that fits
} ApWrapMode;

typedef struct {
    int count;                          // number of lines (>= 1)
    int len;                            // length of the source text
    uint16_t start[AP_TEXT_MAX_LINES];  // index of the first char of each line
} ApText;

// Recompute the whole line table for `width` columns per line. `text` must stay
// alive while the table is used. Safe against overflow: extra lines are dropped
// if the table fills up.
void ap_text_reflow(ApText *t, const char *text, int len, int width,
                    ApWrapMode mode);

// Chars on line `ln`, excluding the trailing newline.
int ap_text_line_len(const ApText *t, const char *text, int ln);

#endif
