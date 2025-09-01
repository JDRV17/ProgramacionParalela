#ifndef PPMIMAGE_H
#define PPMIMAGE_H

#include <pthread.h>
#include <cstdio>
#include "image.h"

class PPMImage : public Image {
private:
    int* pixels;  
    void applyKernel(const float kernel[3][3]);
    
public:
    PPMImage();
    ~PPMImage();
    
    void load(FILE* input) override;
    void save(FILE* output) override;
    void applyFilter(const char* filterType) override;
    void applyKernelToRegion(const float kernel[3][3], int startY, int endY, int startX, int endX, pthread_mutex_t* mutex);
    void loadFromData(const char* magic, int width, int height, int maxColor, int* pixels) override;
    
    int getPixel(int index) const { return pixels[index]; }
    int* getPixels() const { return pixels; }
};

#endif