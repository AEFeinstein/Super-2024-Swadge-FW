# MIDI Specifications {#MIDI}

This page describes the technical details of the 2025 Swadge's support for
the MIDI protocol. Note that while the Swadge only supports the instruments,
controllers, and other features described in this document, it can still
play any MIDI file, even ones containing unsupported commands. Unsupported
commands are simply ignored and the rest of the file will play normally.

## Utilities

For editing MIDI files, [signal](https://signal.vercel.app) is a browser-based MIDI editor. It supports uploading custom SoundFont
files (`.sf2` format). It can also connect to physical MIDI devices such as keyboards, as well as both the physical Swadge and the
Swadge Emulator when the "MIDI Player" mode is active and in Streaming mode.

While Signal is capable of editing MIDI files, it does not offer a straightforward way to assign instruments in banks other than
the default General MIDI bank (Bank 0). To make this easier, [midi-assign](https://dylwhich.com/midi-assign/) is another
browser-based tool made specifically for modifying instrument, bank, and track-to-channel assignments within MIDI files. It
supports the Swadge instrument banks natively, so there is no need to know their numeric IDs.

## Basics

The Swadge's 8-bit audio synthesizer supports up to 16 MIDI channels, with channels 10 and 11 reserved for
percussion (the drum kit). By default, channel 1 through 9 are configured to use the [MAGFest instruments](#MAGPrograms)
in order.

## Features

* 24-voice polyphony shared between non-percussion channels
* 8 additional voices reserved for percussion only channel (channels 10/11 by default)
* Pitch bend
* AfterTouch

## Instrument Banks (Programs)

### General MIDI (Bank 0)
For the list of General MIDI instruments available, see [Wikipedia](https://en.wikipedia.org/wiki/General_MIDI#Program_change_events).
Note that the sound produced for these instruments is a wavetable rather than full-length samples, so they sound more like a retro
keyboard than the high-fidelity samples you might hear from a modern soft synthesizer.

#### GM-Compatible Drum Kit
The Bank 0 Drum Kit is a set of custom drum sounds that follows the standard General MIDI note-to-drum mappings, which can be found
on [Wikipedia](https://en.wikipedia.org/wiki/General_MIDI#Percussion) and is commonly supported by MIDI devices and software. Note
that some drum sounds may be incomplete or not yet supported. This drum kit is available on MIDI Channel 10.

### MAGFest Instruments (Bank 1) {#MAGPrograms}
These instruments are mainly basic wave shapes, with some extra goodies thrown in.

| Program# | Name            |
| -------- | ----------------|
|        0 | Square Wave     |
|        1 | Sine Wave       |
|        2 | Triangle Wave   |
|        3 | Sawtooth Wave   |
|        4 | "MAGFest Wave"  |
|        5 | "MAGStock Wave" |
|        6 | Noise           |
|        7 | Square Wave Hit |
|        8 | Noise Hit       |

#### Donut Swadge Drum Kit
The Bank 1 Drum Kit includes a range of noise-based drum sounds originally featured on the [King Donut Swadge](https://swadge.com/donut/).
By default, this drum kit is available on MIDI Channel 11.

| Note     | Note Number | Description       |
| -------- | ----------- | ----------------- |
| B1 to B2 | 35 to 47    | Donut Drum Kit #1 |
| C2 to C4 | 48 to 60    | Donut Drum Kit #2 |
| C♯4      | 61          | WOAAAGGHHH        |
| D4       | 72          | Donut "MAG"       |
| D♯4      | 73          | Donut "Fest"      |

### MMX Instruments (Bank 2) {#MMXPrograms}
These instruments are based on the MMX SoundFont.
| Program# | Name             |
| -------- | ---------------- |
|       11 | Vibraphone       |
|       17 | Organ            |
|       24 | Acoustic Guitar  |
|       29 | Overdrive Guitar |
|       30 | Distorted Guitar |
|       36 | Slap Bas         |
|       38 | Synth Bass       |
|       48 | Strings          |
|       55 | Orchestra Hit    |
|       62 | Synth Brass      |
|       80 | Square Wave      |
|       81 | Saw Wave         |
|       82 | Synth Lead       |
|       83 | Synth Lead 2     |
|      119 | Reverse Cymbal   |

#### MMX Drum Kit
The MMX drum kit is also available for percussion channels using Bank 2.

| Note       | Note Number | Description  |
| ---------- | ----------- | ------------ |
| C1         | 36          | Kick         |
| C♯1        | 37          | High-Q       |
| D1 to E1   | 38 to 40    | Snare        |
| F1         | 41          | Power Snare  |
| F♯1 to G2  | 42 to 55    | Open Hi-Hat  |
| G♯2 to C4  | 56 to 72    | Crash Cymbal |
| C♯4 to C♯5 | 73 to 85    | Synth Tom    |
| D5 to F♯6  | 86 to 102   | Tom          |


## MIDI Continuous Controllers

Continuous controller numbers (CC#) are shown here starting from 0.
For controllers that have two numbers in the `MIDI CC#` column, the first number is the "Coarse" (sometimes called "MSB") control,
and the second is the "Fine" (sometimes called "LSB") control. These are really two separate controllers analogous to a set of
fine and coarse adjustment knobs on a real audio device, but some editing software might display these as a single control instead.

| MIDI CC# | Name            | Value Range             | Description | Notes |
| -------- | --------------- | ----------------------- | ----------- | ----- |
| 0, 32    | Bank Select     | 0-1                     | Selects the instrument bank used. | Only Fine adjustment (32) is used, Coarse (0) is ignored |
| 7, 39    | Channel Volume  | 0-127                   | Sets volume level for only this channel | Only Coarse adjustment (7) is used, Fine (39) is ignored |
| 64       | Hold Pedal      | 0-63 (off), 64-127 (on) | When the hold pedal is enabled, all notes that are currently playing are sustained until the hold pedal is released, as well as any notes that begin playing while the hold pedal is already down. | |
| 66       | Sustenuto Pedal | 0-63 (off), 64-127 (on) | When the sustenuto pedal is enabled, only the notes that are currently playing are sustained until the sustenuto pedal is released. Notes that begin playing while the sustenuto pedal is already down are not affected. | |
| 72       | Release Time    | 0-127                   | Set the length of time (in 10ms increments) it will take for a note to fade out and stop playing after it is released. | |
| 73       | Attack Time     | 0-127                   | Set the length of time (in 10ms increments) it will take for a note to reach its maximum volume after it starts playing. | |
| 75       | Decay Time      | 0-127                   | Set the length of time (in 10ms increments) it will take for a note to fade from its maximum volume to its sustain volume. | |
| 76       | Sustain Level   | 0-127                   | Set the volume level the note will reach at the end of the decay time, and will be maintained as long as the note is held on. | |
| 6, 38    | Data Entry      | 0-16383                 | Set the value of the registered or non-registered parameter that was selected previously using the `Registered Parameter` or `Non-registered Parameter` controllers. | |
| 96       | Data Button Increment | 0 (not used)      | Increments the value of the registered or non-registered parameter by one | |
| 97       | Data Button Decrement | 0 (not used)      | Decrements the value of the registered or non-registered parameter by one | |
| 98, 99   | Non-registered Parameter | 0-16383        | Selects a non-registered parameter to be changed with the `Data Entry`or `Data Button` controllers. | |
| 101, 100 | Registered Parameter | 0-16383            | Selects a registered parameter value to be changed with the `Data Entry` or `Data Button` controllers. | |


## MIDI Non-registered Parameters

These parameters can be changed by first using the `Non-registered Parameter` controller to select a specific parameter, then
using the `Data Entry` or `Data Button` controllers to actually change the parameter's value.

| MIDI NRPN# | Name       | Value Range     | Description                                                                |
| ---------- | ---------- | --------------- | -------------------------------------------------------------------------- |
| 10         | Percussion | 0 (off), 1 (on) | Sets whether this channel plays a drum kit instead of a pitched instrument |

## MIDI System-Exclusive (SysEx) Parameters

The Swadge does not currently support any SysEx commands of its own, but it does support the following Universal SysEx commands.

| Name       | Data (Hex)                   | Description |
| ---------- | ---------------------------- | ----------- |
| GM Enable  | <pre>F0 7E 7F 09 01 F7</pre> | Enables General MIDI Mode. All channels are fully reset, and set to use Bank 0, Program 0 ("Acoustic Grand Piano"), and Channel 10 only set to Percussion mode. |
| GM Disable | <pre>F0 7E 7F 09 00 F7</pre> | Disables General MIDI Mode. All channels are fully reset, and set to use Bank 1. Channels 1 through 9 are set to use Programs 0 through 8, respectively. Channel 10 is set to Percussion mode, and set to Bank 0 (General MIDI Compatible Drum kit). Channel 11 is set to Percussion mode, and set to Bank 1 (Donut Swadge Drum kit). Channels 12 through 16 **are currently** set to Bank 0, Program 0 ("Acoustic Grand Piano"), but **note** that this may change in the future and is not a guarantee. |
