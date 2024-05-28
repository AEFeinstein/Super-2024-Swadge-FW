#!/usr/bin/env python3
import math

A0 = 21
A4 = 69
note_names = ["A","A#","B","C","C#","D","D#","E","F","F#","G","G#"]

def note_name(num):
    if num < 0:
        return f"n{num:d}"
    else:
        semitones = (num - A0)
        note = note_names[semitones % 12]
        octave = semitones // 12
        return f"{note}{octave}"

note_errs = []

FRAC_BITS = 16
FRAC_DIV = (1 << FRAC_BITS)

def calc_pitch_float(n):
    f = 440 * 2**((n-A4)/12)
    dec_part = int(f)
    frac_bits = int(round((f % 1) * FRAC_DIV))
    actual = (dec_part << FRAC_BITS | frac_bits)

    return actual, f

def gen_note_table():
    maxerr = 0
    maxerrnote = None

    print("static const uq16_16 noteFreqTable[] = {")
    for n in range(128):
        #f = 440 * 2**((n-A4)/12)
        actual, f = calc_pitch_float(n)
        name = note_name(n)
        pad = " " * (5 - len(str(int(f))))
        error = ((actual / 256.0) - f) / f * 100
        olderror = (round(f) - f) / f * 100

        dec_bits = (actual & 0xFFFF00) >> 8
        frac_bits = (actual & 0x0000FF)

        note_errs.append((abs(olderror), name))
        if abs(error) > abs(maxerr):
            maxerr = error
            maxerrnote = name

        #print(f"    ({dec_bits} << 8){pad}| {frac_bits:3}, // {name:4s} = {f:.3f} Hz ({error:+.4f}% error, old={olderror:+.4f}%)")
        print(f"    0x{actual:08x}, // {name:4s} = {f:.3f} Hz")
    print("};")

def gen_bend_table():
    # Pick an arbitrary starting point to calculate the ratios
    _, freq = calc_pitch_float(A4)

    # I could actually get away with uq1.31 and get so much precision from [0, 2) but it's excessive
    print("// Multiply a frequency by this value to bend it by a number of cents")
    print("// uint32_t bentPitch = (uint32_t)(((uint64_t)pitch * bendTable[bendCents + 100]) >> 24)")
    print("static const uq24_8 bendTable[] = {")
    for n in range(-100, 101):
        _, nfreq = calc_pitch_float(A4 + (n / 100.0))
        ratio = nfreq / freq
        bin_ratio = (int(ratio) << 24) | int(round(((ratio % 1) * (2**24 * 1.0))))
        print(f"    0x{bin_ratio:08x}, // {n:+d} cents => {ratio:0.5f}")
    print("};")

def gen_rms_table():
    print("static const uq16_16 rmsTable[] = {")
    for n in range(32):
        val = 1.0/math.sqrt(n) if n > 0 else 0
        intval = (int(val) << 16) | int((val % 1) * 65536)
        print(f"    0x{intval:08x}, // 1 / sqrt({n}) = {val:.4f}")
    print("};")

gen_note_table()
gen_bend_table()
gen_rms_table()

#print(f"Max error: {maxerr} on note {maxerrnote}")
#print(sorted(note_errs))
