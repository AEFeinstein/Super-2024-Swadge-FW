//==============================================================================
// Includes
//==============================================================================
#include "kooks.h"
#include "mainMenu.h"
// #include "settingsManager.h"
// #include "textEntry.h"
//This is a comment
//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    font_t* ibm;
    int8_t board[12][12]; //If changing these values, also change them in the SetupKooks function, and kooksEnterMode
    /*
        Kooks Board Key:
        
        At all times:
            -2: This square is out of play for the current board size, ignore completely

        During Kook Placement:
            -1: This square is seen by a kook in board placement, do not place a kook here
            0: This square can house a kook
            1-BoardSize: This square is in the numbered cluster
        
        During Play:
            1-BoardSize: Empty square
            1-BoardSize + BoardSize: This square is marked with an x, and cannot house a kook
            1-BoardSize + BoardSize * 2: This square is marked with an X by the player, and should remain if the player un-marks a kook that can see these squares
            1-BoardSize + BoardSize * 3: This square has been marked as a kook by the player
    */
    paletteColor_t zoneColors[12];
    int8_t gameSize;
    vec_t pointer;
    bool gameInProgress;
    bool displayMenu;
} kooks_t;

//==============================================================================
// Const variables
//==============================================================================

static const char kooksModeName[] = "Krazy Kooks";
static const int16_t SCREEN_X = 280;
static const int16_t SCREEN_Y = 240;
static const int16_t PLAY_SIZE = 200;

static const int8_t STARTING_GAME_SIZE = 5;
static const int8_t TILE_MARGIN = 2;
//==============================================================================
// Function Definitions
//==============================================================================
static void kooksEnterMode(void);
static void kooksExitMode(void);
static void kooksMainLoop(int64_t);
static void ClearBoard(kooks_t*);
static void SetupKooks(kooks_t*);
static void PlaceXs(kooks_t*);
static void CheckForSolved(kooks_t*);
//==============================================================================
// Variables
//==============================================================================
swadgeMode_t kooksMode = {
    .modeName                 = kooksModeName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = kooksEnterMode,
    .fnExitMode               = kooksExitMode,
    .fnMainLoop               = kooksMainLoop,
    .trophyData               = NULL,
    .fnBackgroundDrawCallback = NULL,
};
kooks_t* kooks;

//==============================================================================
// Functions
//==============================================================================
static void ClearBoard(kooks_t* k){
    for(int x=0; x<12; x++){
        for(int y=0; y<12; y++){
            k->board[x][y] = -2;
        }
    }

}
static void SetupKooks(kooks_t* k)
{
    int kooksPlaced = 0;
    int tilesMarked = 0;
    do{ //TODO: Come up with a cleaner way to ensure gameSize number of kooks are placed without doing it randomly
        for(int x=0; x<k->gameSize; x++){
            for(int y=0; y<k->gameSize; y++){
                k->board[x][y] = 0;
            }
        }

        kooksPlaced = 0;
        tilesMarked = 0;
        while(tilesMarked < k->gameSize * k->gameSize){
            int x = esp_random() % k->gameSize;
            int y = esp_random() % k->gameSize;
            if(k->board[x][y] == 0){
                kooksPlaced++;
                tilesMarked++;
                k->board[x][y] = kooksPlaced;

                //Once a kook is placed, invalidate all squares it can see
                for(int kingX = -1; kingX<=1; kingX++){
                    for(int kingY = -1; kingY<=1; kingY++){
                        if(kingX==0 && kingY==0){continue;} //Don't touch the kook!
                        if(x+kingX < 0 || y+kingY < 0){continue;} //Don't access the -1th element of k.board!
                        if(k->board[x+kingX][y+kingY] == -2){continue;} //Don't access elements beyond the board!
                        if(k->board[x+kingX][y+kingY] == -1){continue;} //Don't count the tile if it's already marked off!
                        tilesMarked++;
                        k->board[x+kingX][y+kingY] = -1;
                    }
                }

                for(int rook = 0; rook<k->gameSize; rook++){
                    if(rook != y && k->board[x][rook] != -1){
                        tilesMarked++;
                        k->board[x][rook] = -1;
                    }
                    if(rook != x && k->board[rook][y] != -1){
                        tilesMarked++;
                        k->board[rook][y] = -1;
                    }
                }
            }
        }
    }while(kooksPlaced < k->gameSize);

    //The board is now known to contain GameSize kooks, numbered 1-GameSize, all other legal squares -1, all other squares -2
    //Now paint the board by having empty squares pull adjactend kook zones, or having kook zones push into empty space
    tilesMarked = 0;
    while(tilesMarked<k->gameSize*(k->gameSize-1)){
        int x = esp_random() % k->gameSize;
        int y = esp_random() % k->gameSize;

        int direction = esp_random() % 4;
        int deltaX = 0;
        int deltaY = 0;

        switch (direction)
        {
            case 0: deltaX = -1; break;
            case 1: deltaX = 1;  break;
            case 2: deltaY = -1; break;
            case 3: deltaY = 1;  break;
            default:break;
        }

        //Don't check for paint if the space is out of bounds
        if(x+deltaX < 0 || x+deltaX >= k->gameSize){continue;}
        if(y+deltaY < 0 || y+deltaY >= k->gameSize){continue;}

        if(k->board[x][y] == -1 && k->board[x+deltaX][y+deltaY] != -1){
            tilesMarked++;
            k->board[x][y] = k->board[x+deltaX][y+deltaY];
        }else if(k->board[x][y] != -1 && k->board[x+deltaX][y+deltaY] == -1){
            tilesMarked++;
            k->board[x+deltaX][y+deltaY] = k->board[x][y];
        }
    }

    k->gameInProgress = true;
}
static void PlaceXs(kooks_t* k)
{
    //Remove all Xs from the board, then add them back based on where current kooks are
    for(int x=0; x<k->gameSize; x++){
        for(int y=0; y<k->gameSize; y++){
            if(((k->board[x][y]-1) / k->gameSize) == 1 ){ //Remove all automatically placed Xs from the board, leave player Xs
                k->board[x][y] -= k->gameSize;
            }
        }
    }

    //Iterate through the board, find kooks, then add Xs where that kook can see
    for(int x=0; x<k->gameSize; x++){
        for(int y=0; y<k->gameSize; y++){
            if(((k->board[x][y]-1) / k->gameSize) == 3){ //If we've found a kook
                int zone = ((k->board[x][y]-1)%k->gameSize)+1;
                
                //Fill in the rook moves
                for(int rook=0; rook<k->gameSize; rook++){
                    if(k->board[x][rook] <= k->gameSize){//Other possibilities are that it has an x, is marked player x, or is a kook
                        k->board[x][rook] += k->gameSize;
                    }
                    if(k->board[rook][y] <= k->gameSize){
                        k->board[rook][y] += k->gameSize;
                    }
                }
                //Fill in the king moves (One step diagonal)
                if(k->board[x-1][y-1] <= k->gameSize){k->board[x-1][y-1]+= k->gameSize;}
                if(k->board[x+1][y+1] <= k->gameSize){k->board[x+1][y+1]+= k->gameSize;}
                if(k->board[x-1][y+1] <= k->gameSize){k->board[x-1][y+1]+= k->gameSize;}
                if(k->board[x+1][y-1] <= k->gameSize){k->board[x+1][y-1]+= k->gameSize;}
                //Fill in the zone
                for(int curX=0; curX<k->gameSize; curX++){
                    for(int curY=0; curY<k->gameSize; curY++){
                        if(k->board[curX][curY] == zone){
                            k->board[curX][curY] += k->gameSize;
                        }
                    }
                }
            }
        }
    }
}
static void CheckForSolved(kooks_t* k)
{
    int count = 0;
    for(int x=0; x<k->gameSize; x++){
        for(int y=0; y<k->gameSize; y++){
            if(((k->board[x][y]-1) / k->gameSize) == 3 ){ //Count the kooks
                count++;
            }
        }
    }
    if(count == k->gameSize){
        ClearBoard(k);
        SetupKooks(k);
    }
}
static void kooksEnterMode(void)
{
    kooks                = heap_caps_calloc(1, sizeof(kooks_t), MALLOC_CAP_8BIT);
    kooks->ibm           = getSysFont();
    kooks->gameSize      = STARTING_GAME_SIZE;

    kooks->displayMenu = false;
    kooks->gameInProgress = false;

    kooks->pointer = (vec_t){
        .x = 0, 
        .y = 0,
    };

    kooks->zoneColors[0] = c300; 
    kooks->zoneColors[1] = c030;
    kooks->zoneColors[2] = c003; 
    kooks->zoneColors[3] = c330, 
    kooks->zoneColors[4] = c033, 
    kooks->zoneColors[5] = c303, 
    kooks->zoneColors[6] = c333, 
    kooks->zoneColors[7] = c140, 
    kooks->zoneColors[8] = c023, 
    kooks->zoneColors[9] = c555, 
    kooks->zoneColors[10] = c555, 
    kooks->zoneColors[11] = c555;

    ClearBoard(kooks);
    SetupKooks(kooks);
}
static void kooksMainLoop(int64_t elapsedUs)
{
    //Clear screen, get a pointer for dynamic text, and set our screen bounds
    drawRectFilled(0, 0, 280, 240, c000);
    //char outNums[32] = {0};
    int xStart = (SCREEN_X - PLAY_SIZE) / 2;
    int yStart = (SCREEN_Y - PLAY_SIZE) / 2;
    int squareSize = PLAY_SIZE / kooks->gameSize;

    //Draw the board as it is
    for(int x = 0; x<kooks->gameSize; x++){
        for(int y = 0; y<kooks->gameSize; y++){

            if(x == kooks->pointer.x && y == kooks->pointer.y){
                drawRectFilled(xStart + squareSize*x, yStart+squareSize*y, xStart+squareSize*(x+1), yStart+squareSize*(y+1),c555); //Replace with a pointer at some point maybe
            }

            drawRectFilled(xStart + squareSize*x + TILE_MARGIN, yStart+squareSize*y + TILE_MARGIN, xStart+squareSize*(x+1)-TILE_MARGIN, yStart+squareSize*(y+1)-TILE_MARGIN, kooks->zoneColors[((kooks->board[x][y]-1) % kooks->gameSize)]);

            if((kooks->board[x][y]-1)/kooks->gameSize == 1 || (kooks->board[x][y]-1)/kooks->gameSize == 2){
                //Draw an X on the square
                drawLine(xStart + squareSize*x + TILE_MARGIN*4, yStart+squareSize*y + TILE_MARGIN*4, xStart+squareSize*(x+1)-TILE_MARGIN*4, yStart+squareSize*(y+1)-TILE_MARGIN*4,c111,0);
                drawLine(xStart+squareSize*(x+1)-TILE_MARGIN*4, yStart+squareSize*y + TILE_MARGIN*4, xStart + squareSize*x + TILE_MARGIN*4, yStart+squareSize*(y+1)-TILE_MARGIN*4,c111,0);
            }else if((kooks->board[x][y]-1)/kooks->gameSize == 3){
                drawRectFilled(xStart + squareSize*x + TILE_MARGIN*4, yStart+squareSize*y + TILE_MARGIN*4, xStart+squareSize*(x+1)-TILE_MARGIN*4, yStart+squareSize*(y+1)-TILE_MARGIN*4,c444);
            }
        }
    }
    

    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            switch (evt.button)
            {
                case PB_LEFT:
                    kooks->pointer.x = (kooks->pointer.x + kooks->gameSize -1) % kooks->gameSize;
                    break;
                case PB_RIGHT:
                    kooks->pointer.x = (kooks->pointer.x + kooks->gameSize +1) % kooks->gameSize;
                    break;
                case PB_UP:
                    kooks->pointer.y = (kooks->pointer.y + kooks->gameSize -1) % kooks->gameSize;
                    break;
                case PB_DOWN:
                    kooks->pointer.y = (kooks->pointer.y + kooks->gameSize +1) % kooks->gameSize;
                    break;    

                case PB_A://Toggle an X, noting that it's placed by player if there wasn't one, and removing that note if the X is otherwise there from a kook seeing it
                {
                    if((kooks->board[kooks->pointer.x][kooks->pointer.y]-1) / kooks->gameSize == 2){
                        kooks->board[kooks->pointer.x][kooks->pointer.y] = ((kooks->board[kooks->pointer.x][kooks->pointer.y]-1)%kooks->gameSize)+1;
                        PlaceXs(kooks);
                    }else if((kooks->board[kooks->pointer.x][kooks->pointer.y]-1) / kooks->gameSize != 3){
                        //If there is no mark, or there is a mark because of an auto-filled X, note that the player wants this to be marked X
                        kooks->board[kooks->pointer.x][kooks->pointer.y] = ((kooks->board[kooks->pointer.x][kooks->pointer.y]-1)%kooks->gameSize)+kooks->gameSize*2+1;
                    }
                    break;
                }
                case PB_B:
                    //Place or remove a kook from a square, iff no other kook can see it. 
                    //If there is a player mark already on the square, remove it, place all kook vision on the board, then check if we can place a kook
                    if((kooks->board[kooks->pointer.x][kooks->pointer.y]-1) / kooks->gameSize == 0){
                        kooks->board[kooks->pointer.x][kooks->pointer.y] += kooks->gameSize * 3;
                        PlaceXs(kooks);
                        CheckForSolved(kooks);
                    }else if((kooks->board[kooks->pointer.x][kooks->pointer.y]-1) / kooks->gameSize == 1){
                        //TODO put something here to show that they're not allowed to do that
                    }else if((kooks->board[kooks->pointer.x][kooks->pointer.y]-1) / kooks->gameSize == 2){
                        //A player has marked this square
                        kooks->board[kooks->pointer.x][kooks->pointer.y] = ((kooks->board[kooks->pointer.x][kooks->pointer.y]-1)%kooks->gameSize)+1;
                        PlaceXs(kooks);
                        if((kooks->board[kooks->pointer.x][kooks->pointer.y]-1) / kooks->gameSize == 0){
                            kooks->board[kooks->pointer.x][kooks->pointer.y] += kooks->gameSize*3;
                            PlaceXs(kooks);
                            CheckForSolved(kooks);
                        }else{
                            //TODO put something here to show that they're not allowed to do that
                        }
                    }else if((kooks->board[kooks->pointer.x][kooks->pointer.y]-1) / kooks->gameSize == 3){
                        //If there is a kook there, remove it and all Xs that were associated with it
                        //This may incur some sort of penalty at some point in the future
                        kooks->board[kooks->pointer.x][kooks->pointer.y] = ((kooks->board[kooks->pointer.x][kooks->pointer.y]-1)%kooks->gameSize)+1;
                        PlaceXs(kooks);
                    }
                    break;
            }
        }
    }
}
static void kooksExitMode(void)
{
    heap_caps_free(kooks);
}