#include <stddef.h>

#include "esp_heap_caps.h"

#include "sm_save.h"

// Allocate memory for a names structure and set its header fields
names_header_t* initNames(uint8_t nameLength, uint16_t numNames) {
    size_t blobLength = sizeof(names_header_t) + nameLength * numNames;
    
    // Bounds check
    // This array needs to fit in an NVS blob
    if(blobLength > NVS_BLOB_MAX_SIZE)
        return NULL;
    
    // Allocate minimum required memory
    names_header_t* namesWithHeader = (names_header_t*) heap_caps_calloc(blobLength, 1, MALLOC_CAP_SPIRAM);
    
    // Set header fields
    namesWithHeader->saveFormat = CURRENT_SAVE_FORMAT;
    namesWithHeader->nameLength = nameLength;
    namesWithHeader->numNames = numNames;
    
    return namesWithHeader;
}

