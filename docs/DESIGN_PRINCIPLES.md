# Swadge Design Principles {#design_principles}

The principles here should guide the development of a Swadge's hardware and firmware. Most of the principles here are guidelines, not hard requirements. When in doubt, talk out ideas in our Slack channel, \#super-circuitboards.

## General Design Principles
- Swadges should not largely rely on at-event infrastructure and should be fully usable after Magfest.
  - This principle may be broken if the feature that requires infrastructure is really cool. Talk it out in the Slack channel first before requiring infrastructure.
- At least one Swadge mode should have wireless connectivity features, like a PvP game
  - Wireless connectivity between Swadges may be less useful after Magfest.
- If a Swadge is turned on and left idle, it should display cool flashing LEDS.
  - This idle mode should be as power-efficient as possible
- Swadges should have at least one secret or easter egg

## Swadge Mode Design Principles
- Modes should try to fit the current theme of the event by:
  - Using themed sprites
  - Using a themed font
  - Using the theme's colors
- Modes should use as much available hardware as possible, including, but not limited to:
  - LED effects
  - Sound effects
  - Accelerometer input
  - Touchpad input
- Modes should be polished, which means:
  - No bugs
  - Looks pretty
  - Has intuitive UI
- Modes should be simple. Overly complex modes are hard to explain and understand, and users will engage with them less.
  - Guided tutorials are good. If there is one, force users to do it once. Tutorials should also be easily accessible to do again.
  - Learning through gameplay by gradually introducing mechanics is great. This is how Nintendo gamifies learning.
- Modes should be designed with a playtime of 1-5 minutes per session.
  - Larger games should have a way to save progress at this interval.
  - Modes should save options between sessions.
- Modes should have an incentive to play and should celebrate🎉 when progress is made. Incentives can be:
  - Unlockable assets (pictures, songs)
  - Unlockable gameplay (new characters, new levels)
  - A story that progresses
  - Getting a high score (this is kind of boring alone, make sure there's flair🎊)

## Hardware Design Principles
- The Swadge shape must be designed to the fit the theme of the event
  - A distinctive theme-specific silhouette is preferable
  - The shape should consider ergonomics
- The Swadge must be designed to be worn on a lanyard.
- There must be two Swadge variants, one for pre-order and one for purchase at Magfest
  - The two Swadge variants should have a sufficiently distinct color scheme.
  - Think of the pre-order Swadge like a Shiny Pokemon
- The Swadge should be physically durable. Use cases include:
  - Dangling on a lanyard when dancing at a concert
  - Being shoved in a tight bag during transit
- If the Swadge uses new or delicate parts, destructively test them until we're confident the failure rate is near zero.
- The Swadge should only include hardware if it is used by modes.
  - For example, do not include an accelerometer if it is used by very few modes.
- If applicable, have a way to calibrate analog inputs.
- Battery life should last the duration of the event with standard use.
- Physical user interface should be intuitive.
  - D-Pad input is common and well understood.
  - A dedicated mode select button is better than a button combo or menu option.
  - Buttons should have silkscreen labels.
- The Swadge may have 3D printable accessories
  - If we sell accessories, they must be budgeted ahead of time and ordered at the same time as the Swadges themselves.
  - If being sold, print files for accessories should be made publicly available two weeks before Magfest.
  - If not being sold, print files can be made available as early as desired.

## Manual Design Principles
- The manual should fit on an 8.5x11" sheet of paper, double sided
  - The sheet of paper may be folded
  - A high-polish manual is hard to make and even harder to sell
- The manual should be posted to https://swadge.com/ in HTML form
  - Posting a PDF of the manual is fine, but making a web friendly version is better
- The manual may be included in the Swadge itself if practical

## Firmware Development Principles
- Before development begins, an outline and feature milestones of the mode should be written down and shared with the team.
- Some milestones should be designated as the 'finished mode.' It's OK to have milestones after this point, like stretch goals.
- At each milestone, the mode should be shown off to the team, to receive feedback and guide development
- Work on optional milestones only after essential milestones are completed
