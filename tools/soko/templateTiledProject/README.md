Open the project 'templateProject.tiled-project' using the most recent version of Tiled tilemap editor.

IF YOUR OBJECTS DO NOT SNAP TO THE CENTER OF THE GRID TILES, GO TO EDIT>PREFERENCE>FINE GRID DIVISIONS AND SET IT TO 2.

All tiles should go in the 'tiles' tilemap layer. Use the 'tilesheet' tileset to place walls, floors, and goals.
All entities should go in the 'entities' object layer. Use the 'objLayers' tileset to place objects with baked-in data.

Entities:

Player:
    Be sure to set the 'gamemode' property.
    Valid values are:
            SOKO_OVERWORLD,
            SOKO_CLASSIC,
            SOKO_EULER,
            SOKO_LASERBOUNCE

Crate:
    The 'sticky' property indicates whether the crate will stick to a player's sprite.
    The 'trail' property indicates whether a crate will leave its own trail in a SOKO_EULER puzzle.

Button:
    The 'playerPress' property indicates whether a player can depress the button.
    The 'cratePress' property indicates whether a crate can depress the button.
    The 'invertAction' property inverts the button's effects on all of its target blocks. For instance, all non-inverted Ghost Blocks targeted by the Button will start intangible.
    The 'stayDownOnPress' property indicates whether the button will remain depressed after its first press after resets once players or crates are removed.
    To target a ghostblock, find the Object ID of the target in Tiled and populate the target#id property with that ID (start at target1id and count up).
    Be sure to set the 'numTargets' property to the number of targeted blocks.

Ghost Block:
    Target a Ghost Block with a Button.
    The 'playerMove' property indicates whether a player can move the ghost block like a crate while in its tangible state.
    The 'inverted' property indicates whether a Ghost block will start intangible (unless the button targeting it is intangible).

Internal Warp and Internal Warp Exit:
    The 'hp' property indicates how many times a Warp can be entered.
    The 'allow_crates' property indicates whether an Internal Warp may pass crates to their destination. Note that the destination will be blocked by a Crate on its destination.
    To target another internal warp, find the Object ID of the target in Tiled and populate the 'target_id' field with that ID.
    Warps may only target Internal Warp and Internal Warp Exit blocks. Internal Warp Exits have no function in gameplay and serve only as destination markers for Internal Warps. To make a 2-Way Portal, have two Internal Warps target one another's IDs.

Laser Emitter/Receiver:
    Be sure to set the 'emitDirection' property.
    Valid values are:
        UP,
        DOWN,
        RIGHT,
        LEFT
    The 'playerMove' property indicates where the Laser Emitter can be pushed by players.

