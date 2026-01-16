#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "sm_monster.h"

monster_instance_t monsters[6];
monster_instance_packed_t packedMonsters[6];

typedef struct __attribute__((packed)) {
    bool seen:1;
    bool caught:1;
} dex_packed_t;

dex_packed_t dex[4];

int main() {
    printf("normal size: %lu\n", sizeof(monster_instance_t));
    printf("packed size: %lu\n", sizeof(monster_instance_packed_t));
    printf("normal in array: %lu\n", sizeof(monsters) / 6);
    printf("packed in array: %lu\n", sizeof(packedMonsters) / 6);
    printf("dex packed: %lu\n", sizeof(dex_packed_t));
    printf("dex: %lu\n", sizeof(dex));
}
