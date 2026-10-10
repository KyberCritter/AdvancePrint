// Frame-driven print-job engine. Drives a full multi-chunk print (INIT -> DATA
// -> ARM -> PRINT -> WAIT) without any UI or GBA headers of its own; the caller
// advances it once per frame and polls the phase. Portable C: only the link
// layer it calls (ap_link_*) is GBA-specific.
#ifndef AP_JOB_H
#define AP_JOB_H

#include <stdint.h>
#include <stdbool.h>
#include "advanceprint/advanceprint_text.h"
#include "advanceprint/advanceprint_page.h"

typedef enum {
    AP_PHASE_IDLE = 0,
    AP_PHASE_RUNNING,
    AP_PHASE_DONE,
    AP_PHASE_ERROR,
    AP_PHASE_CANCELLED,
} ApPhase;

typedef enum {
    AP_OK = 0,
    AP_ERR_NO_PRINTER,  // detection failed
    AP_ERR_EMPTY,       // no content to print
    AP_ERR_LINK_LOST,   // keepalive vanished mid-job
    AP_ERR_STATUS,      // printer reported a fault (see status)
    AP_ERR_BAD_STATE,   // update called on a non-running job
} ApError;

typedef struct {
    // Source: exactly one of these. If `image` is non-NULL the raw 2bpp image
    // (AP_ROW_BYTES per tile row, `image_rows` rows) is printed; otherwise
    // `text` is wrapped by `lines` and composed with the installed font.
    const uint8_t *image;
    int image_rows;
    const char *text;
    const ApText *lines;

    uint16_t settle_frames; // frames to wait after INIT (0 = default 12 ~ 200ms)
    uint8_t exposure;       // print density 0..0x7F (0 = default 0x40)
} ApJobConfig;

typedef struct {
    // --- status (read-only to the caller) ---
    ApPhase phase;
    ApError error;
    uint8_t  status;        // last status byte reported by the printer
    int rows_total;         // content tile rows in the whole job
    int rows_sent;          // tile rows sent so far (progress)
    bool printing;          // printer has reported PRINTING at least once

    // --- internals (do not touch) ---
    uint8_t page[AP_PAGE_BYTES];
    ApJobConfig cfg;
    int state;
    int cur_row;
    int chunk_rows;
    int chunk_pkts;
    int data_idx;
    int settle;
    int wait_frames;
    int bad_polls;
    bool saw_printing;
    bool link_lost;
    bool do_cancel;
} ApJob;

// Probe for a printer (one INQUIRY round trip; no paper used). status may be
// NULL. Requires ap_link_init() to have been called once at startup.
bool ap_job_probe(uint8_t *status);

// Begin a job. Runs printer detection synchronously and, on success, latches
// the first chunk and leaves the job in AP_PHASE_RUNNING. Returns false and
// sets error/phase on failure (no printer or empty content). The config and any
// text/line buffers must stay alive for the job's lifetime.
bool ap_job_begin(ApJob *job, const ApJobConfig *cfg);

// Advance the job by one step (call once per frame). Returns the current phase.
ApPhase ap_job_update(ApJob *job);

// Request cancellation; observed at the next packet boundary.
void ap_job_cancel(ApJob *job);

#endif
