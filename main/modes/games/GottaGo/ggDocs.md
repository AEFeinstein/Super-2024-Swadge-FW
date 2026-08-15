# Gotta Go!

A game where you have to decide which urinal to use.

## Mechanics

Puzzle mechanics:
- Varying number of urinals
- Varying quantity of people peeing
- Varying player character you have to parse (Can wear skirt, be one of 2 heights, etc)
- Different types of people (Weird, Smelly, wearing a skirt)
- Different types of urinals (Wide, skinny, low, out of order, dirty)
- Chicken out: Use a stall

Other
- Timer: Faster you decide the higher you social score
- Score: The better your score, the more socially aware your rating
- Auto-Gen levels: Automatically make more in an endless mode
- Swadgepass: Send social score to other people

Modes:
- Splash
- Main menu
- Game
- Rules
- High score table

Controls: 
- Use one of the slide pads or arrow keys to pick a urinal
- Use A to select

## Scoring

Selection mod:
- More points for being further from other people
- More points if you pick a clean, height matched urinal
- Min points for using an out-of-order urinal
- Stalls are close to minimum points, but preferable to using an out of order urinal

Score calculation
- 1 second grace period
- After 1 second, start decrementing a number
- Score is how much of the number remains over max value times selection mod
- If time runs out you pee your pants and lose
- Given time is based on how far along the player is in sequence