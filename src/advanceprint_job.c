#include <string.h>
#include "advanceprint/advanceprint_job.h"
#include "advanceprint/advanceprint_packet.h"
#include "advanceprint/advanceprint_link.h"

enum {
    JS_INIT,
    JS_SETTLE,
    JS_DATA,
    JS_ARM,
    JS_PRINT,
    JS_WAIT,
};

static void fail(ApJob *j, ApError err)
{
    j->error = err;
    j->phase = AP_PHASE_ERROR;
}

// Margins make chunks join into one continuous document: feed-before on the
// first chunk, feed-after on the last, 0x00 between.
static uint8_t chunk_margins(const ApJob *j)
{
    bool first = (j->cur_row == 0);
    bool last  = (j->cur_row + j->chunk_rows >= j->rows_total);
    return (uint8_t)((first ? 0x10 : 0x00) | (last ? 0x03 : 0x00));
}

// Compose and arm the next chunk: up to one printer buffer (18 rows) trimmed to
// what's left and rounded up to the 2-row band the printer's head advances in.
static void start_chunk(ApJob *j)
{
    int rows = j->rows_total - j->cur_row;
    if (rows > AP_ROWS_PER_PAGE)
        rows = AP_ROWS_PER_PAGE;
    j->chunk_rows = rows;
    j->chunk_pkts = (rows + 1) / 2;           // 2 tile rows per DATA packet

    if (j->cfg.image) {
        memset(j->page, 0, AP_PAGE_BYTES);    // blank-pad an odd final row
        memcpy(j->page, j->cfg.image + (size_t)j->cur_row * AP_ROW_BYTES,
               (size_t)rows * AP_ROW_BYTES);
    } else {
        ap_page_compose_text(j->page, j->cfg.text, j->cfg.lines, j->cur_row);
    }
    j->data_idx = 0;
    j->link_lost = false;
    j->state = JS_INIT;
}

bool ap_job_probe(uint8_t *status)
{
    return ap_link_detect(status);
}

bool ap_job_begin(ApJob *j, const ApJobConfig *cfg)
{
    memset(j, 0, sizeof(*j));
    j->cfg = *cfg;
    if (j->cfg.exposure == 0)
        j->cfg.exposure = 0x40;
    if (j->cfg.settle_frames == 0)
        j->cfg.settle_frames = 12;            // ~200ms at 60 Hz

    j->phase = AP_PHASE_IDLE;
    j->error = AP_OK;

    if (!ap_link_detect(&j->status)) {
        fail(j, AP_ERR_NO_PRINTER);
        return false;
    }

    if (j->cfg.image) {
        if (j->cfg.image_rows <= 0) {
            fail(j, AP_ERR_EMPTY);
            return false;
        }
        j->rows_total = j->cfg.image_rows;
    } else {
        if (!j->cfg.text || !j->cfg.lines || j->cfg.lines->count <= 0) {
            fail(j, AP_ERR_EMPTY);
            return false;
        }
        j->rows_total = j->cfg.lines->count;
    }

    j->cur_row = 0;
    j->rows_sent = 0;
    j->printing = false;
    j->phase = AP_PHASE_RUNNING;
    start_chunk(j);
    return true;
}

ApPhase ap_job_update(ApJob *j)
{
    if (j->phase != AP_PHASE_RUNNING)
        return j->phase;
    if (j->do_cancel) {
        j->phase = AP_PHASE_CANCELLED;
        return j->phase;
    }

    uint8_t pkt[AP_PACKET_MAX];
    uint8_t ka = 0, st = 0;

    switch (j->state) {
    case JS_INIT: {
        int n = ap_packet_build(AP_CMD_INIT, NULL, 0, pkt);
        ap_link_send(pkt, n, AP_HALF_NORMAL, &ka, &st);
        if (ka != AP_KEEPALIVE) {
            j->status = st; j->link_lost = true;
            fail(j, AP_ERR_LINK_LOST);
            break;
        }
        j->settle = j->cfg.settle_frames;
        j->state = JS_SETTLE;
        break;
    }

    case JS_SETTLE:
        if (j->settle-- > 0)
            break;
        j->state = JS_DATA;
        break;

    case JS_DATA: {
        int n = ap_packet_build(AP_CMD_DATA,
                                j->page + (size_t)j->data_idx * AP_DATA_PER_PKT,
                                AP_DATA_PER_PKT, pkt);
        ap_link_send(pkt, n, AP_HALF_NORMAL, &ka, &st);
        if (ka != AP_KEEPALIVE || ap_status_error(st)) {
            j->link_lost = (ka != AP_KEEPALIVE);
            j->status = st;
            fail(j, j->link_lost ? AP_ERR_LINK_LOST : AP_ERR_STATUS);
            break;
        }
        j->data_idx++;
        j->rows_sent = j->cur_row + j->data_idx * 2;
        if (j->rows_sent > j->rows_total)
            j->rows_sent = j->rows_total;
        if (j->data_idx >= j->chunk_pkts)
            j->state = JS_ARM;
        break;
    }

    case JS_ARM: {
        int n = ap_packet_build(AP_CMD_DATA, NULL, 0, pkt);  // empty DATA
        ap_link_send(pkt, n, AP_HALF_NORMAL, &ka, &st);
        if (ka != AP_KEEPALIVE) {
            j->status = st; j->link_lost = true;
            fail(j, AP_ERR_LINK_LOST);
            break;
        }
        j->state = JS_PRINT;
        break;
    }

    case JS_PRINT: {
        uint8_t args[4];
        ap_print_args(chunk_margins(j), j->cfg.exposure, args);
        int n = ap_packet_build(AP_CMD_PRINT, args, 4, pkt);
        ap_link_send(pkt, n, AP_HALF_NORMAL, &ka, &st);
        if (ka != AP_KEEPALIVE || ap_status_error(st)) {
            j->link_lost = (ka != AP_KEEPALIVE);
            j->status = st;
            fail(j, j->link_lost ? AP_ERR_LINK_LOST : AP_ERR_STATUS);
            break;
        }
        j->saw_printing = false;
        j->printing = false;
        j->wait_frames = 0;
        j->bad_polls = 0;
        j->rows_sent = j->cur_row + j->chunk_pkts * 2;
        if (j->rows_sent > j->rows_total)
            j->rows_sent = j->rows_total;
        j->state = JS_WAIT;
        break;
    }

    case JS_WAIT: {
        // poll roughly every 3 frames (~50ms) — keeps the printer awake
        if (++j->wait_frames % 3)
            break;
        int n = ap_packet_build(AP_CMD_INQUIRY, NULL, 0, pkt);
        ap_link_send(pkt, n, AP_HALF_NORMAL, &ka, &st);
        bool finished = false;
        if (ka != AP_KEEPALIVE) {
            // replies can garble while the motor runs — tolerate ~2s of
            // silence; after seeing it print, silence means the job is done
            if (++j->bad_polls < 40)
                break;
            if (j->saw_printing) {
                finished = true;
            } else {
                j->status = st; j->link_lost = true;
                fail(j, AP_ERR_LINK_LOST);
                break;
            }
        } else {
            j->bad_polls = 0;
            j->status = st;
            if (ap_status_error(st)) {
                j->link_lost = false;
                fail(j, AP_ERR_STATUS);
                break;
            }
            if (st & AP_STATUS_PRINTING) {
                j->saw_printing = true;
                j->printing = true;
            }
            finished = j->saw_printing ? !(st & AP_STATUS_PRINTING)
                                       : (j->wait_frames > 60 * 30);
        }
        if (finished) {
            j->cur_row += j->chunk_rows;
            if (j->cur_row > j->rows_total)
                j->cur_row = j->rows_total;
            j->rows_sent = j->cur_row;
            if (j->cur_row < j->rows_total)
                start_chunk(j);
            else
                j->phase = AP_PHASE_DONE;
        }
        break;
    }

    default:
        fail(j, AP_ERR_BAD_STATE);
        break;
    }

    return j->phase;
}

void ap_job_cancel(ApJob *j)
{
    j->do_cancel = true;
}
