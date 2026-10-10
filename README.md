# AdvancePrint

A small, reusable **Game Boy Printer** driver for the **Game Boy Advance**.

It communicates with the Game Boy Printer by bit-banging the link port in general-purpose GPIO mode, and offers a frame-driven engine for multi-chunk print jobs (documents longer than one printer buffer).

By Scott Ratchford — [scottratchford.com](https://scottratchford.com)

Licensed under the **GNU GPL v3** — see [LICENSE.md](LICENSE.md).

## Where it came from

AdvancePrint was extracted from GBScribe, my homebrew word processor and pixel-art tool for Game Boy Advance hardware that prints to a real Game Boy Printer. The printer layer started life as a component of that project. This repository is that layer on its own, renamed AdvancePrint and deliberately decoupled from the app. It no longer depends on libtonc, libgba, Maxmod, or any project-specific code.

## Wiring

```txt
GBA ── link port (GPIO mode) ── GBC/GB-Pocket link cable ── Game Boy Printer
```

The cable crosses SO<->SI, so: GBA SC -> printer clock, GBA SO -> printer SIN, GBA SI <- printer SOUT.

## Layers

| Layer | Header | Portability |
| --- | --- | --- |
| Wire (GPIO bit-bang) | `advanceprint/advanceprint_link.h` | GBA-only |
| Protocol (packets/checksums/status) | `advanceprint/advanceprint_packet.h` | portable C |
| Text wrap | `advanceprint/advanceprint_text.h` | portable C |
| Page composition (text/raw -> 2bpp) | `advanceprint/advanceprint_page.h` | portable C |
| Job engine (INIT/DATA/PRINT/WAIT) | `advanceprint/advanceprint_job.h` | portable C (calls the link) |

`advanceprint/advanceprint.h` includes everything. Only `advanceprint_link.c` touches GBA registers; every other translation unit is plain C.

## Quick start

```c
#include <advanceprint/advanceprint.h>

// once at startup:
ap_link_init();

// --- print a raw 160px-wide 2bpp image (320 bytes per tile row) ---
static ApJob job;
const uint8_t *bitmap   = my_image;   // 320 * rows bytes
int            bitmap_h = 18;         // tile rows (8 px each)

ApJobConfig cfg = {
    .image      = bitmap,
    .image_rows = bitmap_h,
    .exposure   = 0x40,               // print density (0 = default)
};
if (!ap_job_begin(&job, &cfg)) {
    // job.error == AP_ERR_NO_PRINTER or AP_ERR_EMPTY
} else {
    while (ap_job_update(&job) == AP_PHASE_RUNNING) {
        // call once per frame; job.rows_sent / job.rows_total for a progress bar
        frame_wait();
    }
}
```

### Printing text

```c
#include <advanceprint/advanceprint.h>

static ApText lines;                  // ~8 KB with the default line budget
static ApJob  job;
const char *text = "Hello, printer!\nSecond line.";

ap_text_reflow(&lines, text, (int)strlen(text), AP_TILES_PER_ROW,
               AP_WRAP_POST_WORD);

ApJobConfig cfg = { .text = text, .lines = &lines };
ap_job_begin(&job, &cfg);             // composes text -> 2bpp with the font
while (ap_job_update(&job) == AP_PHASE_RUNNING)
    frame_wait();
```

The bundled default font is a 128-glyph GB 2bpp font (ASCII in `0x20..0x7E`). Install your own with `ap_page_set_font(font, glyph_count)` before starting a job; glyphs are 16 bytes, indexed by `(unsigned char)c & 0x7F`.

## The job engine

`ap_job_begin()` probes for a printer (one INQUIRY, no paper) and, on success, arms the first chunk. Drive it with `ap_job_update()` **once per frame**. Poll `phase`, `error`, `status`, `rows_sent` / `rows_total`, and `printing` for progress. Data streams as chunks of up to 18 rows (one printer buffer), joined with feed margins so a document prints contiguously. Call `ap_job_cancel()` to stop at the next packet boundary.

## Building

Requires Docker (uses the official `devkitpro/devkitarm` image):

```sh
./build.sh          # -> libAdvancePrint.a
./build.sh clean
```

Then link `libAdvancePrint.a` and add `-Iinclude` to your GBA project.

Host-side tests (plain gcc, in WSL or any POSIX box):

```sh
make -C tests/host run
```

The printer byte stream is verified without hardware by decoding it with mofosyne's independent `GameBoyPrinterDecoderC` (vendored in `tests/host/decoder`) and pixel-comparing the result. The harness checks exact packet framing and checksums, the wrap engine, and round-trips a composed page through the decoder. The generated font source is committed.

## Layout

```txt
src/                 library source (only advanceprint_link.c is GBA-specific)
include/             public headers under advanceprint/
tests/host/          host-side unit tests and the printer decode harness
tests/host/decoder/  vendored mofosyne GameBoyPrinterDecoderC (independent decoder)
```

## Legal

AdvancePrint is free software, licensed under the GNU General Public License v3 — see [LICENSE.md](LICENSE.md).

Not affiliated with, endorsed by, or sponsored by Nintendo. Game Boy, Game Boy Color, Game Boy Advance and Game Boy Printer are trademarks of Nintendo.
