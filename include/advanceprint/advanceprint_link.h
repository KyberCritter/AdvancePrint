// Game Boy Printer link driver: bit-bangs the GBA link port in general-purpose
// GPIO mode. GBA-ONLY — the protocol, page, and job layers are portable.
//
// Wiring (GBC/GB-Pocket cable, crosses SO<->SI):
//   GBA SC -> printer clock, GBA SO -> printer SIN, GBA SI <- printer SOUT.
//
// Call ap_link_init() once at startup before any other ap_* function.
#ifndef AP_LINK_H
#define AP_LINK_H

#include <stdint.h>
#include <stdbool.h>

// Put the link port into GPIO mode and start the delay timer (TM2).
void ap_link_init(void);

// Clock one byte MSB-first at `half_ticks` timer ticks per half period, with
// interrupts masked for the byte. Returns the byte the printer clocked back.
uint8_t ap_link_xfer(uint8_t send, uint16_t half_ticks);

// Send a built packet (ap_packet_build output) of `n` bytes. The printer's
// replies to the final two bytes land in *keepalive / *status (either may be
// NULL).
void ap_link_send(const uint8_t *pkt, int n, uint16_t half_ticks,
                  uint8_t *keepalive, uint8_t *status);

// One INQUIRY round trip; true if a printer answered (keepalive 0x81).
// Uses the safe stock rate. No paper is used.
bool ap_link_detect(uint8_t *status_out);

// Half period in timer ticks (59.6 ns each): 61 us ~ 1024 (8192 Hz stock rate).
// A ~16 kHz "fast" mode existed but failed on real hardware (status 00), so
// everything runs at the stock rate now.
#define AP_HALF_NORMAL 1024

#endif
