#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "image.h"
#include "pgmimage.h"
#include "ppmimage.h"

#define MAX_FILENAME 256
#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "missing input and output paths\n";
        std::cout << "usage:" << argv[0] << " input_image.pgm output_image.pgm" << std::endl;
        std::cout << "or " << argv[0] << "input_image.ppm output_image.ppm" << std::endl;
        return 1;
    }

    Image* image = Image::createFromFile(argv[1]);
    if (image == NULL) {
        std::cout << "Error, incorrect path or incorrect file." << std::endl;
        return 1;
    }
    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        std::cout << "Error, could not open file." << std::endl;
        delete image;
        return 1;
    }
    image->load(file);
    fclose(file);

    FILE *output = fopen(argv[2], "w");
    if (output == NULL) {
        std::cout << "Error, could not open file." << std::endl;
        delete image;
        return 1;
    }
    image->save(output);
    fclose(output);
    
    delete image;
    return 0;
}