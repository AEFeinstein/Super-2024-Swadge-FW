# Gotta Go!

## TODO

1. [x] Add auto flush as an option
2. [x] Update scoring mechanics
  - [x] Adjust score penalties
  - [ ] Penalize picking center urinals 
  - [x] Update calculations
    - [x] Change score for using a stall
    - [x] Diagnose/fix inability to get perfect scores (<1 sec, correct urinal)
3. [x] Fix touch controls
4. [ ] Trophies
  - [x] Beat 10 levels in a row
  - [x] Beat 20 levels in a row
  - [x] Beat 30 levels in a row
  - [x] Beat 15+ levels with a Average of 100%
  - [x] Beat 15+ levels with a Average of 99%
  - [x] Beat 15+ levels with a Average of 95%
  - [x] When all Urinals are occupied or Broken, use a Stall to continue
  - [x] Attempt to use a stall when there's no uses left
  - [x] Pick the worst available option 5x in a row
  - [ ] Activate helper mode (hidden trophy)
  - [x] Find Manifesto (hidden trophy)
  - [x] Play 20 games
5. [x] Refactor
  - [x] Update data struct
  - [x] Move drawing routines to their own file
  - [x] Move all game functions (Score, draw, init, etc) to their own file
  - [x] Use references to cut down on stupid
  - [x] Rename functions/vars/etc to be less stupid
  - [x] Make everything Enums instead of magic numbers
6. [ ] Add settings 
  - [ ] Toggle tutorial/helper mode
  - [ ] Toggle touch input
7. [ ] Tutorial update
8. [ ] Casual Mode
  - [ ] Non-timed, non-scored mode
  - [ ] Casual mode trophies
9. [ ] Swadgepass
  - [ ] Add to packet
  - [ ] Pull from packet
  - [ ] High Score table
10. [ ] Update visuals
  - [ ] Small text font
  - [ ] Large font
  - [ ] Make functions based on current state/overlap drawing routines to cut down on duplication
  - [ ] Add divider to leftmost toilet when < 7 toilets
  - [ ] Fix lose text when quitting on purpose
  - [ ] Add won game display to endcard
  - [ ] Add reminder to drink water to the main menu
  - [ ] Fix shoes to always draw
  - [ ] Merge win/lose screens, add icons for easier visual distinction
  - [ ] Sort strings
11. Sounds
  - [ ] BGM (Waiting on theme team)
  - [ ] SFX
    - [ ] Move
    - [ ] Select

## Known Bugs
- Occasionally, toilets that cannot used are marked with a check
- Sometimes when num of urinals reduces next round, selection pointer is off the end.
- Long toilets do not have a penalty applied
- Puddles overwrite previous penalties
- End urinals aren't incentivized
- Odd-order urinals are valued the same as even order