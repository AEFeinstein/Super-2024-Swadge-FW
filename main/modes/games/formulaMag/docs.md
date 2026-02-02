# Formula MAG

Manage a tiny Formula MAG team to victory in an entirely fictional race series to which any comparisons are purely coincidental. Manage crew, drivers, and the car to make the best team and win the prized MAG cup. Similar to Golden Lap.

## Core gameplay

Players will start at the start of a season as a newcomer to the sport. They only have a limited amount of money to split between 2 drivers, pit crew, and the car itself. Once purchased, they are given some time to prepare for the race. 

During the prep phase, players are able to train their drivers, crew, and purchase/tune/repair the parts for the car. This will increase the stats of each unit at the cost off some money and time. They are also able to take/manage sponsor contracts.

Race day has two sections: Preparation and the Race itself. Prep involves tuning and qualifying. There's a certain amount of time allowed for qualifying, and each time a car goes out, it does an out-lap, flying lap, and in-lap. These burn time. The flying lap is used to determine the order the cars start in for the main race. Parts are still worn during preparation. Tuning also takes time, but allows for improvements to the car. Each time the car goes out, they earn tuning points which can be put toward any of the main stats. Once time is over, there's a choice of initial starting tires, and then the main race begins. Each race has a specific amount of laps, and the fastest car wins. Players can influence the race by choosing when to pit, each driver's pace, and which tires to change to. Once the lead car crosses the line, lapped cars have their race cut short.

Several things can go wrong: The cars can have mechanical failures, the drivers can get sick or be too busy partying, or maybe the car just looses traction and crashes.

After the race, points are awarded. Car damage is assessed, and skills are adjusted. There's three results, the per-race results, the constructor's and the championship.

Once a championship is decided, the player can do another season with their current team, re-up contracts, and continue to improve the vehicle.

## Systems

TODO: Split below tree into individual blocks

- Calendar
  - Race schedule
  - Race results
  - Points
    - Per race
    - Constructors
    - Championship
- Money
  - Sponsors
- Drivers
  - Stat block
    - Aggressiveness (Base Pace)
    - Confidence (Random range adjustment)
    - Experience (Base speed/turning)
  - Training
    - Tiredness
  - Traits
    - Party animal
    - Serial dater
    - Stoic
    - Preferred track style
    - Preferred track
    - Unpref. track style
    - Unpref. track
- Pit Crew
  - Stat block
    - Adaptability (Speed of random event)
    - Experience (Base speed)
    - Reliability (Random range adjustment)
  - Training
    - Tiredness
- Cars
  - Stat block
    - Player visible
      - Aerodynamics (Cornering, Top Speed)
      - Brakes (Braking)
      - Power (Acceleration, Top Speed)
      - Suspension (Braking, Cornering)
      - Weight (Acceleration)
    - Not player visible
      - Acceleration
      - Braking/Heating
        - Speed of decel
        - How fast brake fade happens
      - Top speed 
      - Cornering
  - Parts
    - Purchase
    - Tune
    - Repair
    - Wear
- Race Day
  - Track
    - Weather
    - Map
      - Zoom
    - Passing/Traffic
    - Navigation
    - Pit lanes
    - Issues
      - Flags (yellow/red)
        - Crashes
        - Mechanical failures
        - Fan/child/dog wanders onto track
      - Human factors
        - Sickness
        - Tired
        - High
        - Distracted
    - Position chart
  - Preparation
    - Tuning
    - Qualifying
      - Fastest lap
  - Race
    - Number of laps
    - Pace
      - Subdued (Less wear, slower, crash chance lower)
      - Medium (moderate)
      - Aggressive (More wear, faster, crash chance higher)
    - Pitting
    - Tires
      - Compounds
        - Super soft
        - Soft
        - Medium
        - Hard
        - Super Hard
        - Intermediate
        - Wet
      - Tire wear graph
      - Weather conditions

## Tracks

7 tracks required.

### How Cars move aro8und tracks

Each track is set made up of sections. Each section contains the following data:
- Spline: Used to draw the track
  - Length is a part of spline?
  - Difficulty based on angle
- Target speed into section
- Next section
- Pit section (Null if empty)

Cars go through each section and look ahead either one or two sections to try to optimize strategy (if CPU cycles available). The cars go from section to section, attempting to maximize speed.

Factors that determine how well a car does in a section:
- Driver Skill
- Driver condition
- Car stats: Depending on the 
- Tire compound: A flat value, depends on the weather conditions
- Tire wear: How worn down the tires are. The more wear, the less grip
- Component wear: Some components will slow down the car when worn, like the engine
- Brake fade: Deceleration is hindered, and cars go wider
- Randomness: Random factors 

## Stretch goals

- Custom Team logos
- Deterministic races for sharing between swadges
  - C rand function?
- Competitive multiplayer
- Difficulty levels (Harder is less money, locked crew, etc)
- Swadgesona for manager
- Lots of funny sponsors
- 24 tracks total