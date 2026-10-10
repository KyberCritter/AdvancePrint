#include "advanceprint/advanceprint_link.h"
#include "advanceprint/advanceprint_packet.h"

// Direct GBA register access. The library deliberately does not depend on
// libtonc/libgba headers so it can drop into any devkitARM project (or sit
// alongside those libs without name clashes).
#define AP_REG16(addr) (*(volatile uint16_t *)(uintptr_t)(addr))

#define AP_SIOCNT    AP_REG16(0x04000128)
#define AP_RCNT      AP_REG16(0x04000134)
#define AP_TM2CNT_L  AP_REG16(0x04000108)
#define AP_TM2CNT_H  AP_REG16(0x0400010A)
#define AP_IME       AP_REG16(0x04000208)

#define AP_TM_ENABLE 0x0080

// RCNT data bits: SC=0, SD=1, SI=2, SO=3; direction bits +4 (1 = output).
#define R_GPIO   (1u << 15)
#define R_SC     (1u << 0)
#define R_SO     (1u << 3)
#define R_SC_OUT (1u << 4)
#define R_SO_OUT (1u << 7)

void ap_link_init(void)
{
    AP_SIOCNT = 0;
    // general purpose mode; SC+SO outputs, SI+SD inputs; SC idles HIGH, SO low
    AP_RCNT = R_GPIO | R_SC_OUT | R_SO_OUT | R_SC;

    // TM2: free-running, prescaler 1 (59.6ns/tick), used for polled delays.
    // TM0/TM1 belong to Maxmod's audio FIFO — touching them corrupts audio AND
    // our bit timing (that was the "constant crackle + status 00" hardware bug).
    AP_TM2CNT_H = 0;
    AP_TM2CNT_L = 0;
    AP_TM2CNT_H = AP_TM_ENABLE;
}

static inline void delay_ticks(uint16_t t)
{
    uint16_t start = AP_TM2CNT_L;
    while ((uint16_t)(AP_TM2CNT_L - start) < t) ;
}

uint8_t ap_link_xfer(uint8_t send, uint16_t half_ticks)
{
    uint8_t recv = 0;
    uint16_t base = (AP_RCNT | R_SC) & ~R_SO;   // keep mode/dir bits

    uint16_t ime = AP_IME;
    AP_IME = 0;                                  // no IRQ may split a byte
    for (int bit = 7; bit >= 0; bit--) {
        uint16_t lo = base & ~R_SC;
        if (send & (1 << bit))
            lo |= R_SO;
        AP_RCNT = lo;                            // clock low, data out
        delay_ticks(half_ticks);
        AP_RCNT = lo | R_SC;                     // rising edge: printer samples
        recv = (uint8_t)((recv << 1) | ((AP_RCNT >> 2) & 1));  // we sample SI
        delay_ticks(half_ticks);
    }
    AP_IME = ime;                                // IRQs may run between bytes
    return recv;
}

void ap_link_send(const uint8_t *pkt, int n, uint16_t half_ticks,
                  uint8_t *keepalive, uint8_t *status)
{
    uint8_t last2[2] = {0, 0};
    for (int i = 0; i < n; i++) {
        uint8_t r = ap_link_xfer(pkt[i], half_ticks);
        last2[0] = last2[1];
        last2[1] = r;
    }
    if (keepalive) *keepalive = last2[0];
    if (status)    *status = last2[1];
}

bool ap_link_detect(uint8_t *status_out)
{
    uint8_t pkt[16];
    int n = ap_packet_build(AP_CMD_INQUIRY, 0, 0, pkt);
    uint8_t ka = 0, st = 0;
    ap_link_send(pkt, n, AP_HALF_NORMAL, &ka, &st);
    if (status_out)
        *status_out = st;
    // pull-ups read 0xFF with nothing attached; a real printer answers 0x81
    return ka == AP_KEEPALIVE;
}
