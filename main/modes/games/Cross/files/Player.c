#include <stdlib.h>

typedef struct {
    int positions[2];
    int camera[2];
    int cheat;
    int lastPos[2];
    float speed[2];
    int falling;
    int dash;
    int timer;
    int dir;
    const char* anim;
} Player;

void initPlayer(Player* player, int x, int y, int camX, int camY) {
    player->positions[0] = x;
    player->positions[1] = y;
    player->camera[0] = camX;
    player->camera[1] = camY;
    player->cheat = 1;
    player->lastPos[0] = 0;
    player->lastPos[1] = 0;
    player->speed[0] = 0;
    player->speed[1] = 0;
    player->falling = 0;
    player->dash = 0;
    player->timer = 0;
    player->dir = 0;
    player->anim = "";
}

void movePlayer(Player* player, int h, int states[3]) {
    player->lastPos[0] = player->positions[0];
    player->lastPos[1] = player->positions[1];
    
    if (player->cheat == 1) {
        player->anim = "Idle";
        if (states[2] == 1 && player->falling < 2) {
            player->positions[1] -= 1;
            player->speed[1] = -3.5;
            player->speed[0] *= 1.5;
        }
        if (states[0] == 1) {
            player->dir = 1;
            player->speed[0] -= 0.7;
            player->anim = "Walk";
        }
        if (states[0] == 2) {
            player->dir = 0;
            player->speed[0] += 0.7;
            player->anim = "Walk";
        }
        
        player->speed[1] += 0.3;
        player->speed[0] *= 0.9;
        
        if (player->speed[1] > 3) {
            player->speed[1] = 3;
        }
        
        player->falling += 1;
        
        if (player->positions[1] >= (40 - h)) {
            player->speed[1] = 0;
            player->positions[1] = (40 - h);
            player->falling = 0;
            player->dash = 1;
        } else {
            if (player->speed[1] > 0) {
                player->anim = "Fall";
            } else if (player->speed[1] < 0) {
                player->anim = "Jump";
            }
        }
        
        if (states[2] == 2 && player->dash == 1) {
            if (states[1] == 2) {
                player->speed[1] = -3;
            }
            if (states[1] == 1) {
                player->speed[1] = 3;
            }
            if (states[0] == 2) {
                player->speed[0] = 3;
            }
            if (states[0] == 1) {
                player->speed[0] = -3;
            }
            player->dash = 0;
        }
    } else {
        if (states[0] == 1) {
            player->speed[0] -= 0.7;
        }
        if (states[0] == 2) {
            player->speed[0] += 0.7;
        }
        if (states[1] == 2) {
            player->speed[1] = -3.5;
        }
        if (states[1] == 1) {
            player->speed[1] += 0.7;
        }
        
        player->speed[0] *= 0.8;
        player->speed[1] *= 0.8;
    }
    
    player->positions[0] += (int)player->speed[0];
    player->positions[1] += (int)player->speed[1];
}

void updateCamera(Player* player, int h) {
    int x_center_min = 30, x_center_max = 38;
    int y_center_min = 16, y_center_max = 24;
    
    int distance_x = abs((player->positions[0] + player->camera[0]) - (x_center_min + x_center_max) / 2);
    int distance_y = abs((player->positions[1] + player->camera[1]) - (y_center_min + y_center_max) / 2);
    
    int speed_multiplier_x = 1 + (int)(distance_x * 0.1);
    int speed_multiplier_y = 1 + (int)(distance_y * 0.1);
    
    if ((player->positions[0] + player->camera[0]) > x_center_max) {
        player->camera[0] -= speed_multiplier_x;
    } else if ((player->positions[0] + player->camera[0]) < x_center_min) {
        player->camera[0] += speed_multiplier_x;
    }

    if ((player->positions[1] + (h - 5) + player->camera[1]) < y_center_min) {
        player->camera[1] += speed_multiplier_y;
    } else if ((player->positions[1] + (h - 5) + player->camera[1]) > y_center_max) {
        player->camera[1] -= speed_multiplier_y;
    }
}