#!/usr/bin/env python3

import os
import re
import sys
import wave
import functools
import pdb

from midi.freqs import note_name, note_freq_symbol

PAIR_RE = re.compile('([a-zA-Z0-9_]+)=(.*)')

SAMPLE_RATE = 16384

SAMPLE_NUMBER_KEYS = {"loop_start", "loop_end", "offset", "end"}
NOTE_FREQ_KEYS = {"pitch_keycenter"}
NOTE_ID_KEYS = {"lokey", "hikey", "key"}
CENTS_KEYS = {"tune"}
SECONDS_TO_SAMPLES_KEYS = {"ampeg_attack", "ampeg_decay", "ampeg_delay", "ampeg_hold", "ampeg_release"}
PERCENT_KEYS = {"ampeg_sustain"}
HERTZ_KEYS = {"cutoff", "amplfo_freq"}

# Various mappings for values
ENUM_MAP = {
    "loop_mode": {
        "loop_continuous": 0,
        "loop_once": 1,
        "no_loop": 1,
        "one_shot": 1,
        "loop_sustain": 0, # hmmmmmm
    },
    "fil_type": {
        "lpf_1p": "LOW_PASS_1P",  # one-pole low pass filter (6dB/octave)
        "hpf_1p": "HIGH_PASS_1P", # one-pole high pass filter (6dB/octave)
        "lpf_2p": "LOW_PASS_2P",  # two-pole low pass filter (12dB/octave)
        "hpf_2p": "HIGH_PASS_2P", # two-pole high pass filter (12dB/octave)
        "bpf_2p": "BAND_PASS_2P", # two-pole band pass filter (12dB/octave)
        "brf_2p": "BAND_REJ_2P",  # two-pole band rejection filter (12dB/octave)
    },
}

unknown_keys = set()

def convert_rate_pos(in_pos, in_rate):
    return int(round(in_pos * 16384 / in_rate, 0))

@functools.lru_cache()
def get_sample_rate(name, basedir):
    name = name.replace('\\', os.path.sep).replace('/', os.path.sep)
    with wave.open(name, 'rb') as f:
        #print(f"{name} has rate {f.getframerate()} Hz")
        return f.getframerate()

def get_basedir(basedir, ctl):
    prefix = ctl.get("default_path", "") if ctl else ""
    if prefix:
        return os.path.join(basedir, prefix)
    else:
        return basedir

def convert_value(key, val, ctx, basedir, ctl):
    if key == "tune" or key == "pitch":
        # Fine tuning in +/- cents, up to 100
        return int(val)
    elif key == "sample":
        prefix = ctl.get("default_path", "") if ctl else ""
        path = val.replace('\\', os.path.sep).replace('/', os.path.sep)
        return os.path.join(prefix, path) if prefix else path
    elif key in NOTE_FREQ_KEYS:
        return note_freq_symbol(int(val))
    elif key in NOTE_ID_KEYS:
        return int(val)
    elif key in CENTS_KEYS:
        return int(val)
    elif key in SAMPLE_NUMBER_KEYS:
        return int(val)
    elif key in SECONDS_TO_SAMPLES_KEYS:
        return int(float(val) * SAMPLE_RATE)
    elif key in PERCENT_KEYS:
        return int((2**16) * float(val) / 100.0)
    elif key in ENUM_MAP:
        return ENUM_MAP[key][val]
    elif key in HERTZ_KEYS:
        return int(float(val))

    if key not in unknown_keys:
        unknown_keys.add(key)
        print(f"Unhandled key {key}!", file=sys.stderr)

    return val

def load_sfz(path):
    control = {"infile": path}
    group = {}
    regions = []
    cur = None

    with open(path) as f:
        for line in f.readlines():
            for part in line.split():
                if part.startswith("//"):
                    break
                elif part.startswith("<control>"):
                    cur = control
                elif part.startswith("<group>"):
                    cur = group
                elif part.startswith("<region>"):
                    cur = {}
                    regions.append(cur)
                else:
                    if part.strip():
                        matched = PAIR_RE.match(part)
                        if matched:
                            key = matched.group(1).strip()
                            val = matched.group(2).strip()

                            # allow spaces in some values
                            if part.startswith("default_path") or part.startswith("sample"):
                                val = line[len(key) + 1:-1]

                            if cur is None:
                                print(f"cur is none for {key}={val}", file=sys.stderr)
                            else:
                                cur[key] = val

    return control, group, regions

def symbolify(text):
    out = ""
    start = True
    word_start = False
    for char in text:
        if start and char.isdigit():
            continue
        elif char.isspace() or char == '_' or char == '-':
            word_start = True
        elif word_start and not start:
            out += char.upper()
            word_start = False
            start = False
        else:
            out += char.lower()
            word_start = False
            start = False
    return out

def prettify(text):
    out = ""
    start = True
    word_start = True

    for char in text:
        if start and char.isdigit():
            continue
        elif char.isspace() or char == '_' or char == '-':
            if out and out[-1] != ' ':
                out += " "
            word_start = True
        elif word_start and not start:
            out += char.upper()
            word_start = False
            start = False
        else:
            out += char
            word_start = False
            start = False
    return out

def print_sample(data, basedir, control, indent=8):
    rate = get_sample_rate(data["sample"], get_basedir(basedir, control))
    sample_bin = os.path.basename(data["sample"]).replace("wav", "bin").replace(".", "_").replace("-", "_").upper()
    loop_mode = data.get('loop_mode', 1)
    loop_start = data.get('loop_start', 0) if not loop_mode else 0
    loop_end = data.get('loop_end', 0) if not loop_mode else 0
    pf = " " * indent
    print(f"{pf}.sample = {{")
    print(f"{pf}    .baseNote = {data['pitch_keycenter']},")
    print(f"{pf}    .tune = {data.get('tune', 0)},")
    print(f"{pf}    .fIdx = {sample_bin},")
    print(f"{pf}    .rate = {rate},")
    print(f"{pf}    .loop = {loop_mode},")
    print(f"{pf}    .loopStart = {loop_start},")
    print(f"{pf}    .loopEnd = {loop_end},")
    print(f"{pf}}},")

def print_envelope(data, indent=8):
    pf = " " * indent

    loop = data.get("loop_mode", 1)
    attack = data.get("ampeg_attack", 0)
    decay = data.get("ampeg_decay", 0)
    sustain = data.get("ampeg_sustain", 127)#0 if loop == 1 else 127)
    release = data.get("ampeg_release", 0)

    print(f"{pf}.envelope = {{")
    print(f"{pf}    .attackTime = {attack},")
    print(f"{pf}    .decayTime = {decay},")
    print(f"{pf}    .sustainVol = {sustain},")
    print(f"{pf}    .releaseTime = {release},")
    print(f"{pf}}},")

def compile_sfz(control, group, regions, inpath):
    basedir = os.path.dirname(inpath)
    multi_sample = False

    proc_regions = []

    symbol_name = symbolify(os.path.basename(inpath).replace(".sfz", ""))
    pretty_name = prettify(os.path.basename(inpath).replace(".sfz", ""))
    print("symbol_name =", symbol_name, file=sys.stderr)

    if len(regions) > 1:
        # Multi-sample instrument
        multi_sample = True

    for region in regions:
        # Let anything in <region> override what's in <group>
        raw_data = dict(group)
        raw_data.update(region)
        data = { k: convert_value(k, v, raw_data, basedir, control) for k, v in raw_data.items() }
        proc_regions.append(data)

    if multi_sample:
        print(f"const noteSampleMap_t {symbol_name}SampleMap[] = {{")
        for data in proc_regions:
            lokey = data.get("lokey", data.get("key"))
            hikey = data.get("hikey", data.get("key"))
            print("    {")
            print(f"        .noteStart = {lokey},")
            print(f"        .noteEnd = {hikey},")
            print_sample(data, basedir, control)
            print_envelope(data)
            print("    },")
        print("};")
        print()

    print(f"const midiTimbre_t {symbol_name}Timbre = {{")
    print(f"    .type = {'MULTI_SAMPLE' if multi_sample else 'SAMPLE'},")
    print(f"    .flags = {'TF_PERCUSSION' if multi_sample else 'TF_NONE'},")
    if multi_sample:
        print("    .multiSample = {")
        print(f"        .map = {symbol_name}SampleMap,")
        print(f"        .count = ARRAY_SIZE({symbol_name}SampleMap),")
        print("    },")
        print_envelope({"ampeg_attack": 0, "ampeg_sustain": 127, "ampeg_release": 0}, indent=4)
    else:
        print_sample(proc_regions[0], basedir, control, indent=4)
        print_envelope(proc_regions[0], indent=4)
    print(f"    .name = \"{pretty_name}\",")
    print("};")

def main(argv):
    if len(argv) > 1:
        for sfz in argv[1:]:
            control, group, regions = load_sfz(sfz)
            compile_sfz(control, group, regions, sfz)

if __name__ == "__main__":
    main(sys.argv)
