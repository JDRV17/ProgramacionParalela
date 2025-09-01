#include "image.h"
#include "pgmimage.h"
#include "ppmimage.h"
#include <cstring>

Image* Image::createFromFile(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return nullptr;
    
    char magic[3];
    fscanf(file, "%2s", magic);
    fclose(file);
    
    if (strcmp(magic, "P2") == 0) return new PGMImage();
    if (strcmp(magic, "P3") == 0) return new PPMImage();
    
    return nullptr;
}