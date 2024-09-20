# MIDI Specifications {#MIDI}

This page describes the technical details of the 2025 Swadge's support for
the MIDI protocol. Note that while the Swadge only supports the instruments,
controllers, and other features described in this document, it can still
play any MIDI file, even ones containing unsupported commands. Unsupported
commands are simply ignored and the rest of the file will play normally.

## Basics

The Swadge's 8-bit audio synthesizer supports up to 16 MIDI channels, with channel 10 reserved for
percussion (the drum kit).

## Features

* 24-voice polyphony shared between non-percussion channels
* 8 additional voices reserved for percussion only channel (channel 10)
* Pitch bend
* AfterTouch

## Instrument Banks (Programs)

### General MIDI (Bank 0)
For the list of General MIDI instruments available, see [Wikipedia](https://en.wikipedia.org/wiki/General_MIDI#Program_change_events).
Note that the sound produced for these instruments is a wavetable rather than full-length samples, so they sound more like a retro
keyboard than the high-fidelity samples you might hear from a modern soft synthesizer.

#### Drum Kit
The Bank 0 Drum Kit follows the key mapping of the General MIDI program sounds, which can be found
on [Wikipedia](https://en.wikipedia.org/wiki/General_MIDI#Percussion). Note that some drum sounds may be incomplete or
not yet supported.

### MAGFest Instruments (Bank 1)
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

#### Drum Kit
The Bank 1 Drum Kit includes a range of noise-based drum sounds originally featured on the [King Donut Swadge](https://swadge.com/donut/).

| Note      | Note Number | Description       |
| --------- | ----------- | ----------------- |
| B1 to B2  | 35 to 47    | Donut Drum Kit #1 |
| C2 to C4  | 48 to 60    | Donut Drum Kit #2 |
| C&sharp;4 | 61          | WOAAAGGHHH        |
| D4        | 72          | Donut "MAG"       |
| D&sharp;4 | 73          | Donut "Fest"      |


## MIDI Continuous Controllers

Continuous controller numbers (CC#) are shown here starting from 0.
For controllers that have two numbers in the `MIDI CC#` column, the first number is the "Coarse" (sometimes called "MSB") control,
and the second is the "Fine" (sometimes called "LSB") control. These are really two separate controllers analogous to a set of
fine and coarse adjustment knobs on a real audio device, but some editing software might display these as a single control instead.

| MIDI CC# | Name            | Value Range             | Description | Notes |
| -------- | --------------- | ----------------------- | ----------- | ----- |
| 0, 32    | Bank Select     | 0-1                     | Selects the instrument bank used. | Only Fine adjustment (32) is used, Coarse (0) is ignored
| 7, 39    | Channel Volume  | 0-127                   | Sets volume level for only this channel | Only Coarse adjustment (7) is used, Fine (39) is ignored
| 64       | Hold Pedal      | 0-63 (off), 64-127 (on) | When the hold pedal is enabled, all notes that are currently playing are sustained until the hold pedal is released, as well as any notes that begin playing while the hold pedal is already down.
| 66       | Sustenuto Pedal | 0-63 (off), 64-127 (on) | When the sustenuto pedal is enabled, only the notes that are currently playing are sustained until the sustenuto pedal is released. Notes that begin playing while the sustenuto pedal is already down are not affected.
| 72       | Release Time    | 0-127                   | Set the length of time (in 10ms increments) it will take for a note to fade out and stop playing after it is released.
| 73       | Attack Time     | 0-127                   | Set the length of time (in 10ms increments) it will take for a note to reach its maximum volume after it starts playing.
| 75       | Decay Time      | 0-127                   | Set the length of time (in 10ms increments) it will take for a note to fade from its maximum volume to its sustain volume.
| 76       | Sustain Level   | 0-127                   | Set the volume level the note will reach at the end of the decay time, and will be maintained as long as the note is held on.
