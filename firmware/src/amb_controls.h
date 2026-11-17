/* SPDX-License-Identifier: GPL-3.0-only */
/* AMB+ absolute-input decoder. The verified FM1 has an absolute MASTER
 * input and quadrature KNOB 1..4 inputs. This decoder also makes the
 * specification's alternative absolute selectors testable without invented
 * ADC wiring. All time is supplied by the caller; never sleeps. */
#ifndef AMB_CONTROLS_H
#define AMB_CONTROLS_H
#include <stdint.h>
typedef struct {
    uint16_t previous, target;
    uint8_t initialized, picked;
} amb_pickup_t;
static void amb_pickup_arm(amb_pickup_t *p, uint16_t target)
{
    p->target = target > 1023u ? 1023u : target;
    p->initialized = p->picked = 0;
}
static int amb_pickup_read(amb_pickup_t *p, int32_t raw)
{
    int32_t delta;
    if (raw < 0 || raw > 1023) return 0;
    delta = raw - p->target;
    if (delta >= -12 && delta <= 12) p->picked = 1;
    if (p->initialized &&
        ((p->previous <= p->target && raw >= p->target) ||
         (p->previous >= p->target && raw <= p->target))) p->picked = 1;
    p->previous = (uint16_t)raw;
    p->initialized = 1;
    return p->picked;
}
typedef struct {
    amb_pickup_t pickup;
    uint32_t since;
    uint8_t bins, selected, candidate;
} amb_selector_t;
static void amb_selector_arm(amb_selector_t *p, uint8_t bins, uint8_t selected)
{
    p->bins = bins < 2 ? 2 : bins > 16 ? 16 : bins;
    p->selected = p->candidate = selected < p->bins ? selected : 0;
    p->since = 0;
    amb_pickup_arm(&p->pickup, (uint16_t)((2u * p->selected + 1u) * 1024u / (2u * p->bins)));
}
/* Returns a settled changed selection, -1 for no change. 12 ADC counts of
 * hysteresis (~1.2% travel), 90 ms final settle; wrapping millisecond clock. */
static int amb_selector_read(amb_selector_t *p, int32_t raw, uint32_t ms)
{
    uint32_t bin, lo, hi;
    if (raw < 0 || raw > 1023) return -1;
    bin = (uint32_t)raw * p->bins / 1024u;
    if (!p->pickup.picked && bin == p->selected) p->pickup.picked = 1;
    if (!amb_pickup_read(&p->pickup, raw)) return -1;
    lo = p->candidate * 1024u / p->bins;
    hi = (p->candidate + 1u) * 1024u / p->bins;
    if ((uint32_t)raw + 12u >= lo && (uint32_t)raw < hi + 12u) bin = p->candidate;
    if (bin != p->candidate) {
        p->candidate = (uint8_t)bin;
        p->since = ms;
    }
    if (p->candidate != p->selected && ms - p->since >= 90u) {
        p->selected = p->candidate;
        return p->selected;
    }
    return -1;
}
#endif
