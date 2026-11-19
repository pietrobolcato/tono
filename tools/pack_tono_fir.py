"""Pack USB resampler coefficients for Tono's flash budget; decode once at USB setup."""
import re
import sys
from pathlib import Path

def pack(source, output):
    rows_text = source.read_text().split("UAC_FIR3[", 1)[1].split("= {", 1)[1].split("};", 1)[0]
    rows = [list(map(int, re.findall(r"-?\d+", row))) for row in re.findall(r"\{([^{}]+)\}", rows_text)]
    assert len(rows) == 24 and all(len(row) == 77 for row in rows)
    headers, data = [], bytearray()
    for row in rows:
        delta = [b - a for a, b in zip(row, row[1:])]
        lo = min(delta)
        width = (max(delta) - lo).bit_length()
        assert width <= 8 and -128 <= lo <= 127
        headers.append((len(data), row[0], lo, width))
        bits = sum((v - lo) << (i * width) for i, v in enumerate(delta))
        data.extend(bits.to_bytes((len(delta) * width + 7) // 8, 'little'))
    data.append(0)  # final two-byte read
    for row, (start, value, lo, width) in zip(rows, headers):
        for i, expected in enumerate(row[1:]):
            pos = i * width
            offset = start + pos // 8
            code = (int.from_bytes(data[offset:offset+2], 'little') >> (pos % 8)) & ((1 << width) - 1)
            value += code + lo
            assert value == expected
    lines = ['/* Generated from uac_fir.h. Fourth differences, packed per tap. */',
             'static const struct {uint16_t start; int16_t first; int8_t low; uint8_t bits;} tono_fir_row[] = {']
    lines += ['    {%d,%d,%d,%d},' % row for row in headers]
    lines += ['};', 'static const uint8_t tono_fir_data[] = {']
    lines += ['    '+','.join(map(str,data[i:i+24]))+',' for i in range(0,len(data),24)]
    output.write_text('\n'.join(lines+['};','']))

if __name__ == '__main__':
    pack(Path(sys.argv[1]),Path(sys.argv[2]))
