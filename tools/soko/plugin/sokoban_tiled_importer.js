//import "classic" sokoban text levels via tiled.
//http://www.sokobano.de/wiki/index.php?title=Level_format
//these levels are plaintext, and use the following scheme:
// # for walls
// @ for the player. + for player on  goal
// . for the goal
// $ for a box, * for box on goal
// space for floor.

//while our game treats empty and wall the same, proper sokoban levels must be enclosed by a wall

// local d = Dialog("Paste Text as level")
//  :label{id="lab1",label="",text="Import tiles."}
//  :text{id="text1"}
//  :label{id="lab3", label="",text="Max supported tilemap size: 255x255"}
//  :separator{}
//  :button{id="ok",text="&OK",focus=true}
//  :button{text="&Cancel" }
//  :show()

tiled.register