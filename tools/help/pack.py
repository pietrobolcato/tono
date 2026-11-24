#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Pack guide.txt into a seekable card directory and one small LCD text stream.

Match lengths and distances use independent prefix-code trees. Repeat the
shortest-path parse with measured symbol costs, so common short words can
beat a longer but distant match. Only the decoder ships on the instrument.
"""
from argparse import ArgumentParser
from collections import Counter, defaultdict
from heapq import heapify, heappop, heappush
from pathlib import Path

HERE = Path(__file__).resolve().parent


def read_cards():
    entries = [b'']  # zero is the unused control
    names = {}
    for line in (HERE / 'guide.txt').read_text().splitlines():
        if not line or line.startswith('#'):
            continue
        if line.startswith('['):
            name = line.removeprefix('[').removesuffix(']')
            assert name.isidentifier() and name not in names
            names[name] = len(entries)
        else:
            entries.append(line.encode('ascii'))
    return names, entries


def find_matches(raw):
    locations = defaultdict(list)
    matches = [{} for _ in raw]
    for pos in range(len(raw)):
        previous = locations[raw[pos:pos + 3]]
        for start in reversed(previous):
            length = 3
            while (length < 257 and pos + length < len(raw)
                   and raw[start + length] == raw[pos + length]):
                length += 1
            for n in range(3, length + 1):
                matches[pos].setdefault(n, pos - start)
        previous.append(pos)
    return matches


def prefix_codes(frequencies):
    heap = [(count, [sym]) for sym, count in frequencies.items()]
    heapify(heap)
    lengths = Counter()
    while len(heap) > 1:
        a, left = heappop(heap)
        b, right = heappop(heap)
        leaves = left + right
        heappush(heap, (a + b, leaves))
        lengths.update(leaves)
    code = previous = 0
    result = {}
    for sym, nbits in sorted(lengths.items(), key=lambda item: (item[1], item[0])):
        code <<= nbits - previous
        result[sym] = (code, nbits)
        previous = nbits
        code += 1
    return result


def match_length(n):
    if n < 11:
        return n - 3, 0, 0
    extra = (n - 3).bit_length() - 1
    return 5 + extra, extra, (n - 3) - (1 << extra)


def tokenize(raw, matches, literal_cost, distance_cost):
    best = [0] * (len(raw) + 1)
    choices = [None] * len(raw)
    for pos in range(len(raw) - 1, -1, -1):
        literal = raw[pos]
        best[pos] = literal_cost[literal] + best[pos + 1]
        choices[pos] = (literal,)
        for n, distance in matches[pos].items():
            sym, extra, value = match_length(n)
            distance_bits = distance.bit_length() - 1
            cost = (literal_cost[128 + sym] + extra + distance_cost[distance_bits]
                    + distance_bits + best[pos + n])
            if cost < best[pos]:
                best[pos] = cost
                choices[pos] = (128 + sym, extra, value, distance_bits,
                                distance - (1 << distance_bits), n)
    pos = 0
    while pos < len(raw):
        token = choices[pos]
        yield token
        pos += token[-1] if len(token) > 1 else 1


def encode(raw):
    matches = find_matches(raw)
    literal_cost = [8] * 256
    distance_cost = [4] * 16
    for _ in range(6):
        seq = list(tokenize(raw, matches, literal_cost, distance_cost))
        literal = prefix_codes(Counter(t[0] for t in seq))
        distance = prefix_codes(Counter(t[3] for t in seq if len(t) > 1))
        literal_cost = [literal.get(i, (0, 16))[1] for i in range(256)]
        distance_cost = [distance.get(i, (0, 16))[1] for i in range(16)]
    bits = []

    def write(value, width):
        bits.extend((value >> i) & 1 for i in range(width - 1, -1, -1))

    for token in seq:
        write(*literal[token[0]])
        if len(token) > 1:
            _, extra, value, distance_bits, distance_value, _ = token
            write(value, extra)
            write(*distance[distance_bits])
            write(distance_value, distance_bits)
    bits.extend([0] * (-len(bits) % 8))
    stream = [sum(v << (7 - j) for j, v in enumerate(bits[i:i + 8]))
              for i in range(0, len(bits), 8)]
    return literal, distance, stream


def array(typ, name, values):
    rows = [','.join(map(str, values[i:i + 20])) for i in range(0, len(values), 20)]
    return f'static const {typ} {name}[]={{\n    ' + ',\n    '.join(rows) + '\n};\n'


def tree(name, codes):
    # Canonical codes need only a count for each bit length and the
    # symbols in code order, shared by the writer and firmware reader.
    ordered = sorted(codes, key=lambda s: (codes[s][1], codes[s][0]))
    lengths = Counter(n for _, n in codes.values())
    counts = [lengths[n] for n in range(1, max(lengths) + 1)]
    return array('uint8_t', 'help_' + name, [len(counts)] + counts + ordered)


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="verify the checked-in firmware catalog")
    args = parser.parse_args()
    names, entries = read_cards()
    raw = b'\0'.join(entries) + b'\0'
    literal, distance, stream = encode(raw)
    out = '/* SPDX-License-Identifier: GPL-3.0-only */\n/* Generated by tools/help/pack.py. Edit guide.txt. */\n'
    out += 'enum {\n' + ''.join(f'    H_{name} = {i},\n' for name, i in names.items()) + '};\n'
    out += tree('literal', literal) + tree('distance', distance)
    out += array('uint8_t', 'help_data', stream)
    out += f'#define HELP_TEXT_CAP {max(map(len, entries)) + 1}\n#define HELP_RAW_SIZE {len(raw)}\n#define HELP_COUNT {len(entries)}\n'
    offset = 0
    blocks = []
    for i, entry in enumerate(entries):
        if i % 8 == 0:
            blocks.append(offset)
        offset += len(entry) + 1
    assert len(raw) < 65536
    out += array('uint16_t', 'help_blocks', blocks)
    path = HERE.parent.parent / 'firmware/src/amb_help_data.h'
    if args.check:
        if path.read_text() != out:
            raise SystemExit('Regenerate firmware/src/amb_help_data.h with tools/help/pack.py')
    else:
        path.write_text(out)


if __name__ == '__main__':
    main()
