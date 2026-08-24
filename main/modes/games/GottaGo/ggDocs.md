# Gotta Go!

## TODO

1. [x] Add auto flush as an option
2. [x] Update scoring mechanics
  - [x] Adjust score penalties
  - [x] Penalize picking center urinals 
  - [x] Update calculations
    - [x] Change score for using a stall
    - [x] Diagnose/fix inability to get perfect scores (<1 sec, correct urinal)
3. [x] Fix touch controls
4. [x] Trophies
  - [x] Beat 10 levels in a row
  - [x] Beat 20 levels in a row
  - [x] Beat 30 levels in a row
  - [x] Beat 15+ levels with a Average of 100%
  - [x] Beat 15+ levels with a Average of 99%
  - [x] Beat 15+ levels with a Average of 95%
  - [x] When all Urinals are occupied or Broken, use a Stall to continue
  - [x] Attempt to use a stall when there's no uses left
  - [x] Pick the worst available option 5x in a row
  - [x] Activate helper mode (hidden trophy)
  - [x] Find Manifesto (hidden trophy)
  - [x] Play 20 games
5. [x] Refactor
  - [x] Update data struct
  - [x] Move drawing routines to their own file
  - [x] Move all game functions (Score, draw, init, etc) to their own file
  - [x] Use references to cut down on stupid
  - [x] Rename functions/vars/etc to be less stupid
  - [x] Make everything Enums instead of magic numbers
6. [x] Add settings 
  - [x] Toggle tutorial/helper mode
  - [x] Toggle touch input
7. [x] Tutorial update
  - [x] Add tutorial images
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
  - [x] Make functions based on current state/overlap drawing routines to cut down on duplication
  - [x] Add divider to leftmost toilet when < 7 toilets
  - [x] Fix lose text when quitting on purpose
  - [x] Add won game display to endcard
  - [x] Add reminder to drink water to the main menu
  - [x] Fix shoes to always draw
  - [x] Merge win/lose screens, add icons for easier visual  distinction
  - [x] Sort strings
  - [x] Pretty up pause menu
11. Sounds
  - [ ] BGM (Waiting on theme team)
  - [ ] SFX
    - [ ] Move
    - [ ] Select
12. Misc
  - [ ] Use LEDs

## Known Bugs
- Sometimes when num of urinals reduces next round, selection pointer is off the end.