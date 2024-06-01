#!/usr/bin/env python3
import math
import random

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

def note_symbol(num):
    semitones = (num - A0)
    note = note_names[semitones % 12]
    octave = semitones // 12

    name_symbol = note.replace("#", "_SHARP_")
    octave_symbol = f"_MINUS_{-octave}" if octave < 0 else str(octave)

    return f"FREQ_{name_symbol}{octave_symbol}".replace("__","_")

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

    define_lines = []
    freq_array_lines = []

    for n in range(128):
        #f = 440 * 2**((n-A4)/12)
        actual, f = calc_pitch_float(n)
        name = note_name(n)
        symbol = note_symbol(n)
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
        define_lines.append(f"#define {symbol:20s} 0x{actual:08x} // = {f:.3f} Hz")
        freq_array_lines.append(f"    {symbol},")

    for line in define_lines:
        print(line)

    print()

    print("static const uq16_16 noteFreqTable[] = {")
    for line in freq_array_lines:
        print(line)
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

def primes(n): # simple sieve of multiples
    odds = range(3, n+1, 2)
    sieve = set(sum([list(range(q*q, n+1, q+q)) for q in odds], []))
    return [2] + [p for p in odds if p not in sieve]

def gen_dither_table():
    eightbit_primes = primes(256)
    random.seed(1337)
    random.shuffle(eightbit_primes)

    print("// Apply a random offset to each oscillator to maybe make it less likely for waves to \"stack\" exactly")
    print("static const uint8_t oscDither[] = {")
    for i, v in enumerate(eightbit_primes):
        if i > 0 and ((i+1) % 10) == 0:
            print(f"{v:3},")
        else:
            print(f"{v:3}, ", end="")
    print()
    print("};")

gen_note_table()
print()

gen_bend_table()
print()

print("#ifdef OSC_DITHER")
gen_dither_table()
print("#endif")

#print(f"Max error: {maxerr} on note {maxerrnote}")
#print(sorted(note_errs))
