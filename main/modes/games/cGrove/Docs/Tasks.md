# Tasks

## General

- [ ] Refactor into multiple files
    - [x] mode_cGrove.h: Only critical initalization logic and main game loop
    - [ ] cGrove_Online.h: Online functionality
    - [x] cg_Garden.h: Main grove functionality
    - [ ] cg_Fight.h: Fighting game
    - [ ] cg_Perform.h: Musical performance
    - [ ] cg_Race.h: Race game
    - [ ] cg_Chowa.h: Chowa functionality
    - [x] prettyKeyboard.h: Text entry
- [x] Create github "issue" for project

## Menu

### Functionality
- [x] Text entry system
    - [x] Use old text entry to make a nice looking keyboard
- [ ] Notifications of changed values
- [ ] Inventory viewer
- [ ] Online
    - [ ] Profile viewer
        - [ ] Show username/mood
        - [ ] Show Chowa/Accomplisments/Stats
    - [ ] Set sharable Chowa
    - [ ] See recently downloaded profiles
    - [ ] Turn on/off online features

### Assets
- [ ] Images
    - [ ] Menu BG
    - [ ] Profile BG1
    - [ ] Profile BG2 (Subprofile)
    - [ ] Keyboard BG
- [ ] Audio
    - [ ] Menu BGM (Borrow from other games)
    - [ ] Selection SFX
    - [ ] Option change SFX
- [ ] Font
    - [ ] Collect Sonic font
    - [ ] Create/Use font closer to Sonic style for menus

## Chowa

### Design
Chowa have a pudgy two-tone body and head, two arms and two legs, two eyes, a tail and a mouth. The mouth has a default expression based on personality but changes based on mood. 

Need to get this drawn up as a concept.

### Research
- [ ] Palette for colors
    - If not palettes, will need to use coposite shapes

### Functionality
- [ ] Age restrictions
    - [ ] Learning slows down once an adult
    - [ ] Children have nerfed stats
    - [ ] Eggs (hatch)
        - [ ] Throw against wall
        - [ ] Timer
        - [ ] Other Chawo sing next to it reduces timer
- [ ] Grove
    - [ ] State machine for behavior (See [Diagram](#state-diagram))
        - [ ] Standing
        - [ ] Eating (Requires food)
        - [ ] Walking
        - [ ] Running
        - [ ] Swimming
        - [ ] Climbing
        - [ ] Singing (Solo and group)
        - [ ] Talking (Group)
        - [ ] Sitting
        - [ ] Jump
        - [ ] Sparring/Fighting/shadowboxing (solo and group)
        - [ ] Item specific
            - [ ] Drawing
            - [ ] Attacking with toy
            - [ ] Throwing object
            - [ ] Reading
    - [ ] Inter-Chawo Interactions
        - [ ] Compatibility mechanic (Cha + how similar stats are)
        - [ ] Breeding (High compatibility + adults)
    - [ ] Chowa player interactions
        - [ ] Kindess/Abuse scale
        - [ ] Willingness to respond to player's actions and accept gifts

#### State Diagram

[Diagram]()

### Assets
- [ ] Images
    - [ ] High Rez Chowa image for art
    - [ ] Egg/Nut to hatch from
    - [ ] Poses
        - [ ] Standing
        - [ ] Eating (animation cycle)
        - [ ] Walking (animation cycle)
        - [ ] Running (animation cycle)
        - [ ] Swimming (animation cycle)
        - [ ] Climbing (animation cycle)
        - [ ] Singing (animation cycle)
        - [ ] Talking (animation cycle)
        - [ ] Dancing x 4 (animation cycle)
        - [ ] Sitting
        - [ ] Jump
        - [ ] Figthing
            - [ ] Kick
            - [ ] Punch
            - [ ] Heatbutt OR Tail-whack
        - [ ] Item specific
            - [ ] Drawing (animation cycle)
            - [ ] Toy weapon slashes (animation cycle)
            - [ ] Tossing
            - [ ] Reading
    - [ ] Facial expressions
        - [ ] Smile
        - [ ] Frown
        - [ ] Scowl
        - [ ] Open
        - [ ] Open/Smile
        - [ ] Closed/normal
        - [ ] Smile/Teeth
    - [ ] Eyes
        - [ ] Open
        - [ ] Closed
- [ ] Audio
    - [ ] Footsteps
    - [ ] 2-5 Effort noises (Fighting, Swimming, Climbing, etc)
    - [ ] 2-5 Pain noises (Getting hit, being tossed, etc)
    - [ ] 2-5 vocalizations (Singing, talking to other Chawo, etc)
    - [ ] Interaction noise (small "pok" sound. Reuse menu sound?)
- [ ] Other
    - [ ] Chawo State machine diagram

## Items

### Assets
- [ ] Images
    - [ ] Crayons
        - [ ] Crayon Pango sprite
        - [ ] Crayon Poe sprite
        - [ ] Crayon Pixel sprite
        - [ ] Crayon Garbotnik sprite
    - [ ] Book
    - [ ] Knife
    - [ ] Ball
    - [ ] Bat
    - [ ] Sword

TODO: Enumerate items

## Inventory

### Functionality
- [ ] Holds up to 255 items
- [ ] Holds a high number of rings
    - [ ] Rings can be used to buy things
    - [ ] Rings can be gained by Chowa gitin gthem to player when they have high kindness
    - [ ] Rings are given as 1st, 2nd and 3rd place prizes in games

### Assets
- [ ] Images
    - [ ] Rings

## Grove

### Functionality
- [ ] Move hand around the screen
- [ ] Scroll around grove by moving hand to edge of screen
- [ ] Select object with A
- [ ] Hold A to pick up object
- [ ] Press B on item to put it away
- [ ] If a held A (Grab) is released while moving, held object is tossed
- [ ] Use cursor on Inventory button to take item from inventory
- [ ] Use cursor on return button to return to main menu
- [ ] Selecting a Chowa pets the Chowa
- [ ] Chowa, food, and items can be picked up and thrown
- [ ] Guest Chowa (Online)

See [Chowa](#chowa) for Chowa behaviors inside the grove.

### Assets
- [ ] Images
    - [ ] Grass tile
    - [ ] Water tile
    - [ ] Trees trunks as walls
    - [ ] UI Hand open
    - [ ] UI Hand closed
    - [ ] Back icon
    - [ ] Inventory icon
- [ ] Audio
    - [ ] Grove BGM
    - [ ] Footsteps
    - [ ] Splashing noises

See [Chowa](#chowa) for assets required for Chowa inside Grove.

## Fighting

TBD

### Functionality
- 

### Assets
- 

## Racing

TBD

### Functionality
- 

### Assets
- 

## Performance

### Functionality
- [ ] Play a song/part of a song
- [ ] Chowa sings along
- [ ] Player presses arrow keys to match with a DDR style game to dance
    - [ ] SPD stat expands hit timing
    - [ ] AGI Increases score gain
- [ ] A button can be pressed when the A icon flashes to sing
    - [ ] CHA increases score gain of this action
- [ ] Pause button quits game
- [ ] Score board with fake NPC scores

### Assets
- [ ] Images
    - [ ] Stage
        - [ ] Microphone
        - [ ] Curtains
    - [ ] Game
        - [ ] Arrow
        - [ ] A prompt
- [ ] Audio
    - [ ] 5 songs from past years/current year
- [ ] Font
    - [ ] Font for "Nice!" "Good!" "Awesome!" hit markers

## Stores

### Functionality
- [ ] Show icon of item, Name, price, and amount of rings/coins
- [ ] Scroll through items one at a time with up and down arrows
- [ ] B button goes back to main menu
- [ ] A button selects item to purchase (if have enough coin)

### Assets
- [ ] Images
    - [ ] Grocery BG
    - [ ] Toy store BG
    - [ ] Book store BG

## School

### Functionality
- [ ] Requires a fee
- [ ] Chowa read a book at the desk
- [ ] Display a timer until done
- [ ] One Chowa at once
- [ ] Selection menu

### Assets
- [ ] Images
    - [ ] Schoolroom
    - [ ] Desk
    - [ ] Chalkboard