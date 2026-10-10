#include "advanceprint/advanceprint_text.h"

#define AP_NL 0x0A

void ap_text_reflow(ApText *t, const char *text, int len, int width,
                    ApWrapMode mode)
{
    int n = 0;
    int cap = AP_TEXT_MAX_LINES;

    t->len = len;
    t->start[n++] = 0;

    int col = 0;          // chars on current line so far
    int last_space = -1;  // index of the last space seen on this line

    for (int i = 0; i < len; i++) {
        char c = text[i];
        if (c == AP_NL) {
            if (n < cap)
                t->start[n++] = (uint16_t)(i + 1);
            col = 0;
            last_space = -1;
            continue;
        }
        if (col >= width) {
            if (c == ' ' && mode == AP_WRAP_POST_WORD) {
                // spaces may overflow past the margin; renderers clip them
                last_space = i;
                continue;
            }
            int brk;
            if (mode == AP_WRAP_POST_WORD && last_space >= 0)
                brk = last_space + 1;   // break after the last space
            else
                brk = i;                // hard break before this char
            if (n < cap) {
                t->start[n++] = (uint16_t)brk;
            } else {
                break;                  // table full: drop the rest
            }
            col = i - brk;
            last_space = -1;
        }
        if (c == ' ')
            last_space = i;
        col++;
    }
    t->count = n;
}

int ap_text_line_len(const ApText *t, const char *text, int ln)
{
    int s = t->start[ln];
    int e = (ln + 1 < t->count) ? t->start[ln + 1] : t->len;
    // a trailing newline terminates the line but isn't part of its content
    if (e > s && e - 1 < t->len && text[e - 1] == AP_NL)
        e--;
    return e - s;
}
