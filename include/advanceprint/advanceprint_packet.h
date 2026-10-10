// Game Boy Printer packet building. PORTABLE C — no GBA headers, so this
// compiles for the ROM and for the host test harness.
#ifndef AP_PACKET_H
#define AP_PACKET_H

#include <stdint.h>
#include <stdbool.h>

#define AP_CMD_INIT    0x01
#define AP_CMD_PRINT   0x02
#define AP_CMD_DATA    0x04
#define AP_CMD_BREAK   0x08
#define AP_CMD_INQUIRY 0x0F

// status byte bits (printer -> GBA)
#define AP_STATUS_CHECKSUM_ERR (1 << 0)
#define AP_STATUS_PRINTING     (1 << 1)
#define AP_STATUS_DATA_FULL    (1 << 2)
#define AP_STATUS_UNPROCESSED  (1 << 3)
#define AP_STATUS_PACKET_ERR   (1 << 4)
#define AP_STATUS_PAPER_JAM    (1 << 5)
#define AP_STATUS_OTHER_ERR    (1 << 6)
#define AP_STATUS_LOW_BATTERY  (1 << 7)

#define AP_KEEPALIVE 0x81       // printer's reply in the keepalive slot

#define AP_MAX_PAYLOAD 0x280    // 640 bytes = 40 tiles = 160x16 px per DATA packet

// header(6) + payload + checksum(2) + keepalive/status slots(2)
#define AP_PACKET_MAX (6 + AP_MAX_PAYLOAD + 2 + 2)

// Build a full packet into `out` (>= AP_PACKET_MAX bytes when payload is max).
// Returns the total number of bytes to clock out. The final two bytes are 0x00
// placeholders; the printer answers keepalive/status in those transfer slots.
int ap_packet_build(uint8_t cmd, const uint8_t *payload, int len, uint8_t *out);

// Offset of the keepalive / status reply bytes within a built packet.
#define AP_REPLY_KEEPALIVE(total) ((total) - 2)
#define AP_REPLY_STATUS(total)    ((total) - 1)

// PRINT command payload: sheets=1, margins hi=before lo=after, palette 0xE4,
// exposure 0..0x7F.
void ap_print_args(uint8_t margins, uint8_t exposure, uint8_t out[4]);

// True if the status byte reports a printer-side fault (not just "busy").
static inline bool ap_status_error(uint8_t status)
{
    return (status & (AP_STATUS_CHECKSUM_ERR | AP_STATUS_PACKET_ERR |
                      AP_STATUS_PAPER_JAM | AP_STATUS_OTHER_ERR)) != 0;
}

#endif
