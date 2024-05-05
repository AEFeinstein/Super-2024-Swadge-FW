# Chawo Grove
A small pet simulator for Swadge 2025. 

The game is a small one-screen game that allows you to raise a pet Chawo (final name TBD) that you can put into competitions, hang out with friends, dress up, and more!

Compare to Chao garden for SA2

## What is a Chawo?
A small, defenseless creature that needs your help to become... something. A fighter, maybe? A speed demon? A Singer? It's up to you to help them reach their full potential. 

[Chawo](Chawo.jpg) #FIXME: Need image

Raise them well, as the evil Dr. Garbotnik is also raising his own Chawo, and he can't be up to any good.

Be sure to take care of your Chawo or they might run away to a better home.

Each Swadge can have up to 5 Chawo at once.

### Numbers

#### Stats
- Color
- Stat block
	- Age
		- Egg (0.0-0.05)
		- New Born (0.06-0.10)
		- Child (0.11-0.35)
		- Adult (0.36-0.85)
		- Elderly (0.86-1.0)
	- Age scale (0-1.0, min 3 days, max 4)
	- Strength (0-1.0)
	- Agility (0-1.0)
	- Speed (0-1.0)
	- Charisma (0-1.0)
	- Stamina (0-1.0)

#### Personality/Mood
- Moods (Enum: text, expression, conditions)
	- Happy
	- Angry
	- Sad
	- Confused
	- Fearful
	- Surprised
	- Disgusted
- Personality (Stat growth modifiers)
	- Shy 
	- Brash
	- Boring 
	- Dumb
	- Cry Baby
	- Smart 
	- Overly Cautious 
	- Careless 
	- Kind
	- Aggressive 

### Raising Chawo
- Breeding
- Feeding (Fruits/Vegetables/Hard drives/Cooked meals)
- Schools to raise stats
- Shops (Gifts/food/Chawo eggs)
- - Affection
	- Pet
	- Kick/Toss
	- Gifts

## Items

### Aesthetics
- Cosmetics (~20 Hats/Shoes)
	- Cosmetics from previous games (Chozo gear?)
	- Online/Event cosmetics

### Gifts
- Crayons
- Ball
- Toy sword
- A *real* knife
- Cake
- Souffle
- Stat book (Str/Agi/Spd/Cha/Sta)

## Competitions
Competitions need to have NPCs to play against, or you can play against other players. Final NPC is a barely disguised Garbotnik.

### NPCs
- Pixel
- Poe
- Pango
- Garbotnik
- Add more from previous years

### Fighting
Fighting is a glorified game of five move rock-paper-scissors. Moves:
- Punch 
- Kick
- Headbutt
- Fast Punch
- Leaping kick

Chart showing who wins
| Player 1\Player 2 | Punch | F. Punch | Kick | L. Kick | Headbutt |
| ----------------- | ----- | -------- | ---- | ------- | -------- |
| Punch    | Draw | P2 | P1 | P2 | P1 |
| F. Punch | P1 | Draw | P2 | P1 | P2 |
| Kick     | P2 | P1 | Draw | P2 | P1 |
| L. Kick  | P1 | P2 | P1 | Draw | P2 |
| Headbutt | P2 | P1 | P2 | P1 | Draw |

Stages:
- Both Chawo start completely unready
- Depending on speed stat, they get ready for an attack
- Once ready, they wait for a short time
	- If both Chawo are ready before the time period ends, regular RPS happens
	- If one is unready, the unready one takes extra damage
- RPS is checked
- Damage is calculated based on Strength stat of attacking Chawo
- Agility is checked to see if the losing Chawo dodges
- Based on activities, drain some stamina
- Once stamina runs out, sit down and attempt to recover some stamina
- Readiness is reset

Player actions:
- Cheer Chawo: When regaining stamina, increase rate of regen
- Tell Chawo to get up: When regaining stamina, encourage them to stand back up and prepare to attack
- Pick fighting move: When preparing, influence move Chawo uses
- Dodge: When preparing, influence Chawo to dodge. Does less damage if preparing to dodge
- Forfeit: Any time, give up. Ask multiple times.

### Singing
Play a song/a portion of a song and make a random sequence of button presses.

Sequence:
- Song starts, score is set to 0
- Show a arrow/button on screen
- Buttons are "singing"
- Arrows are dancing
- Faster you press button, the higher the score
- Add anticipation? (DDR style, change from faster to more precise)

Chawo Stats:
- Str: No effect
- Agi: Dance modifier
- Cha: Sing modifier
- Spd: How long you have to press buttons
- Sta: No effect

### Racing
A footrace along an obstacle course.

Controls:
- Player can cheer the Chawo a few times per race, determined by Mood
- Player can give up anytime

Sequence:
- Running on path (Sta)
	- Chawo runs along path. 
	- Cheering provides a boost in speed.
	- Combined running value below 0.5 adds chance of tripping
- Balance beam (Agi)
	- Chawo tightrops.
	- Cheering provides a boost in speed.
	- Agi score < 0.5 has a chance of falling off
- Swimming (Str+Sta)
	- Chawo Swims in pool
	- Cheering provides a boost in speed.
	- Combined value below 0.5 adds chance of floundering
- Climbing (Str)
	- Chawo Climbs a rock wall
	- Cheering provides a boost in speed.
	- Str score < 0.5 has a chance of stalling temporarily

Chawo Stats:
- Str: Adds modifier to Swimming and Climbing
- Agi: Adds modifier to Balance beam
- Cha: No effect
- Spd: Speed modifier for all events
- Sta: Adds modifier to Running and Swimming

## Other features

### Online functionality
- Player ID (Name/Emotion tag/pronouns)
- Competitions

### Text entry
- Player and Chawo name entry

## Required Assets

### Art
- NPCs 
	- Pixel
	- Poe
	- Pango
	- Garbotnik
	- Selected from previous games (+random Chawo)
- Chawo (All poses) (Assumes palette swap)
	- Standing
	- Walking
	- Running
	- Eating
	- Jumping up and down
	- Climbing
	- Kick
	- Punch
	- Heatbutt (reuse Run?)
	- Trip (fall over)
	- Floundering
- Chawo Expressions
	- Happy
	- Angry
	- Sad
	- Confused
	- Fearful
	- Surprised
	- Disgusted
- Emotes
	- Anger particles/Steam
	- Sparkles
	- Exclamation point
- Grove background
- Concert stage
- Stage for fights (Dojo)
- Race course (Track/Balance beam/Pool/Climbing wall) Needs to loop
- Distance markers for races
- UI art
	- Message panel
	- Font
	- Buttons
	- Face buttons (A, B, C, Up, Down, Left, Right)

### Music
Borrow as much from other projects as possible. Combine race and fight music? If the sound circuit is created, might need to source some music.

- Simple grove theme
- Action theme (Reuse from other games?)
- Song for singing competition (Use songs from previous years)

### SFX
- Interface beep
- Chawo interaction "pok"
- Chawo effort noises (2-5)
- Chawo vocalizations (2-5)
- Chawo pain noise
- Chawo eating
- Water splashes
- Footsteps

## Notes
- Maybe no palette swapping. If not, use abstract shapes and position them?
- LEDs will change colors based on competition
