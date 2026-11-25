#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Losslessly pack the piano, sax, violin and cello in seekable 256-frame blocks."""
import io
from amb_pcm_octets import SRC, assets
from amb_pcm_pack import write_array


def block(samples, period):
    n = len(samples)
    winner = None
    # Predict from the last sample, its slope, or a neighbouring full/half cycle.
    for mode in range(42):
        prev = 128
        prev2 = 128
        error = 0
        residuals = []
        lag = period + ((mode-2) >> 3) - 2
        fraction = ((mode-2) >> 1) & 3
        for i, value in enumerate(samples):
            pred = prev
            if mode == 1 and i >= 2:
                pred += prev - prev2
            elif mode >= 2 and i > lag:
                ref = samples[i-lag]
                ref += ((samples[i-lag-1] - ref)*fraction + 2) // 4
                pred = ref + (error if mode & 1 else 0)
                error = value - ref
            diff = value - pred
            residuals.append(2*diff if diff >= 0 else -2*diff-1)
            prev2, prev = prev, value
        cost = 0
        groups = []
        for j in range(0, n, 32):
            r = residuals[j:j+32]
            costs = [len(r)*(k+1) + sum(x >> k for x in r) for k in range(8)]
            costs.append(8*len(r))
            k = min(range(9), key=costs.__getitem__)
            cost += costs[k]
            groups.append((k,r))
        if winner is None or cost < winner[0]:
            winner = (cost, mode, groups)
    _, mode, groups = winner
    packed = bytearray([mode])
    widths = sum(k << (4*j) for j,(k,r) in enumerate(groups))
    packed.extend(widths.to_bytes(4,'little'))
    bits = count = 0
    for j,(k,r) in enumerate(groups):
        if k == 8:
            bits |= int.from_bytes(bytes(samples[j*32:j*32+len(r)]),'little') << count
            count += 8*len(r)
            continue
        for x in r:
            count += x >> k
            bits |= (1 | ((x % (1<<k))<<1)) << count
            count += 1+k
    packed.extend(bits.to_bytes((count+7)//8, 'little'))
    return packed


def main():
    out = io.StringIO()
    out.write('/* CC0 acoustic recordings, packed losslessly by tools/amb_pcm_rice.py. */\n')
    for name, period, samples in assets(include_piano=True):
        if name not in ('tenor', 'piano', 'violin', 'cello'):
            continue
        data = bytearray()
        offsets = []
        for start in range(0, len(samples), 256):
            offsets.append(len(data))
            chunk = samples[start:start+256]
            chunk += [chunk[-1]] * (-len(chunk) % 8)
            data.extend(block(chunk, period))
        data.extend(bytes(3))  # predictor bit lookahead
        assert len(data) < 65536
        write_array(out, 'uint16_t', 'amb_rice_'+name+'_offsets', offsets)
        write_array(out, 'uint8_t', 'amb_rice_'+name, data)
        print(name, len(samples), len(data) + 2*len(offsets), flush=True)
    (SRC/'amb_pcm_rice_data.h').write_text(out.getvalue())


if __name__ == '__main__':
    main()
