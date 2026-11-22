"""Lossless, independently addressable PCM groups for Tono's sustained notes."""

# Strides near the excerpt's period keep amplitudes in a group close together.
# They only affect the storage size. Playback always uses the original samples.
PERIODS = {"cello": 46, "piano": 23, "flute": 19}
GROUP_SIZE = 16


def write_array(out, c_type, name, values):
    out.write(f"static const {c_type} {name}[{len(values)}] = {{\n")
    for i in range(0, len(values), 16):
        out.write("    " + ",".join(map(str, values[i:i+16])) + ",\n")
    out.write("};\n")


def write_offsets(out, symbol, offsets):
    # One absolute address per eight groups and four bits per bit width.
    # Every group occupies one base byte and two more bytes per bit of width.
    widths = [(b-a-1)//2 for a,b in zip(offsets,offsets[1:])]
    packed = [sum(w << (4*n) for n,w in enumerate(widths[i:i+8]))
              for i in range(0,len(widths),8)]
    write_array(out, "uint16_t", symbol + "_offsets", offsets[:-1:8])
    write_array(out, "uint32_t", symbol + "_widths", packed)


def write_pcm(out, name, values):
    symbol = "amb_pcm_" + name
    if name not in PERIODS:
        write_array(out, "uint8_t", symbol, values)
        return
    period = PERIODS[name]
    data = bytearray()
    offsets = []
    for start in range(0, len(values), GROUP_SIZE * period):
        for phase in range(min(period, len(values) - start)):
            group = values[start+phase:start+GROUP_SIZE*period:period]
            base = min(group)
            width = (max(group) - base).bit_length()
            packed = sum((value - base) << (i * width) for i, value in enumerate(group))
            offsets.append(len(data))
            data.append(base)
            data.extend(packed.to_bytes(2 * width, "little"))
    offsets.append(len(data))
    assert offsets[-1] < 65536
    data.extend(b"\x00\x00")  # bounded two-byte read, including constant final groups
    out.write(f"enum {{ {symbol}_length = {len(values)}, {symbol}_period = {period} }};\n")
    write_offsets(out,symbol,offsets)
    write_array(out, "uint8_t", symbol, data)
