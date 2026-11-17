/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "../firmware/src/amb_controls.h"
int main(void)
{
    amb_pickup_t p;
    amb_selector_t s;
    unsigned n, i, ms = 0;
    amb_pickup_arm(&p, 700);
    assert(!amb_pickup_read(&p, 120));
    assert(!amb_pickup_read(&p, -1));
    assert(!amb_pickup_read(&p, 1024));
    assert(amb_pickup_read(&p, 900)); /* crossing, even a quick sweep */
    amb_pickup_arm(&p, 200);
    assert(!amb_pickup_read(&p, 900)); /* no false crossing on retarget */
    assert(amb_pickup_read(&p, 205));
    for (n = 6; n <= 8; n += 2) {
        amb_selector_arm(&s, n, 0);
        assert(amb_selector_read(&s, 1023, ms += 100) == -1);
        assert(!s.pickup.picked);
        amb_selector_read(&s, 0, ms += 100);
        assert(s.pickup.picked);
        for (i = 1; i < n; i++) {
            int raw = (2 * i + 1) * 1024 / (2 * n);
            assert(amb_selector_read(&s, raw, ms += 100) == -1);
            assert(amb_selector_read(&s, raw, ms += 100) == (int)i);
        }
        for (i = n - 1; i-- > 0;) {
            int raw = (2 * i + 1) * 1024 / (2 * n);
            assert(amb_selector_read(&s, raw, ms += 100) == -1);
            assert(amb_selector_read(&s, raw, ms += 100) == (int)i);
        }
    }
    amb_selector_arm(&s, 8, 0);
    amb_selector_read(&s, 64, 0);
    for (i = 0; i < 1000; i++)
        assert(amb_selector_read(&s, 124 + i % 9, i) == -1); /* boundary noise */
    amb_selector_read(&s, 200, 1000);
    amb_selector_read(&s, 400, 1010);
    amb_selector_read(&s, 700, 1020);
    assert(amb_selector_read(&s, 700, 1109) == -1);
    assert(amb_selector_read(&s, 700, 1110) == 5); /* final selection only */
    amb_selector_read(&s, 900, UINT32_MAX - 40);
    assert(amb_selector_read(&s, 900, 60) == 7); /* ms wrap */
    puts("AMB+ controls: pickup, retarget, 8/6 bins, both directions, jitter, final settle, clock wrap PASS");
    return 0;
}
