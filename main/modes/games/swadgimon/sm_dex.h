#ifndef _SM_DEX_H_
#define _SM_DEX_H_

// Data in RAM for each species in the encyclopedia
struct {
    bool seen;
    bool caught;
} dex_species_t;

// Data saved to NVS for each species in the encyclopedia
struct __attribute__((packed)) {
    bool seen:1;
    bool caught:1;
} dex_species_packed_t;

#endif
